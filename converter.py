from neo4j import GraphDatabase
import json

NEO4J_URI = "bolt://localhost:7687"
NEO4J_USER = "GoydaGoyda"
NEO4J_PASSWORD = "GoydaGoyda"

STRUCTURIZR_JSON_PATH = "workspace.json"

driver = GraphDatabase.driver(NEO4J_URI, auth=(NEO4J_USER, NEO4J_PASSWORD))

def parse_tags(tag_string):
    """
    Разбивает строку тегов, разделённых запятыми,
    обрезает пробелы по краям каждого элемента.
    """
    return [tag.strip() for tag in tag_string.split(",") if tag.strip()]

def parse_technologies(tech_string):
    """
    Аналогичное разбиение для технологий,
    если в DSL перечисляются несколько технологий через запятую.
    """
    return [tech.strip() for tech in tech_string.split(",") if tech.strip()]

def load_structurizr_model(json_path):
    with open(json_path, 'r', encoding='utf-8') as file:
        model = json.load(file)
    return model

def extract_elements_and_relationships(model):
    """
    Собирает из JSON все элементы (узлы) и отношения, 
    включая вложенные контейнеры, компоненты и deploymentNode.
    """
    elements = []
    relationships = []

    # 1) Persons
    for person in model['model'].get('people', []):
        elements.append({
            'id': person['id'],
            'type': 'Person',
            'name': person['name'],
            'properties': person.get('properties', {}),
            'tags': parse_tags(person.get('tags', ''))  # разбиение тегов
        })
        relationships.extend(person.get('relationships', []))

    # 2) SoftwareSystems + вложенные контейнеры и компоненты
    for system in model['model'].get('softwareSystems', []):
        elements.append({
            'id': system['id'],
            'type': 'SoftwareSystem',
            'name': system['name'],
            'properties': system.get('properties', {}),
            'tags': parse_tags(system.get('tags', ''))
        })
        relationships.extend(system.get('relationships', []))

        # Контейнеры
        for container in system.get('containers', []):
            elements.append({
                'id': container['id'],
                'type': 'Container',
                'name': container['name'],
                'parentId': system['id'],
                'properties': container.get('properties', {}),
                'tags': parse_tags(container.get('tags', '')),
                # Разбиваем technology на список (если несколько через запятую)
                'technology': parse_technologies(container.get('technology', ''))
            })
            relationships.extend(container.get('relationships', []))

            # Компоненты
            for component in container.get('components', []):
                elements.append({
                    'id': component['id'],
                    'type': 'Component',
                    'name': component['name'],
                    'parentId': container['id'],
                    'properties': component.get('properties', {}),
                    'tags': parse_tags(component.get('tags', '')),
                    'technology': parse_technologies(component.get('technology', ''))
                })
                relationships.extend(component.get('relationships', []))

    # 3) DeploymentNodes (рекурсивный разбор)
    for deployment_node in model['model'].get('deploymentNodes', []):
        process_deployment_node(deployment_node, elements, relationships)

    # 4) Глобальные relationships (если есть)
    relationships.extend(model['model'].get('relationships', []))

    return elements, relationships

def process_deployment_node(node, elements, relationships, parent_id=None):
    """
    Рекурсивно обрабатывает deploymentNode, 
    а также вложенные containerInstance и infrastructureNode.
    """
    node_id = node['id']
    node_type = 'DeploymentNode'
    node_name = node['name']
    properties = node.get('properties', {})
    tags = node.get('tags', '')
    environment = node.get('environment', '')

    # Сам deploymentNode
    elements.append({
        'id': node_id,
        'type': node_type,
        'name': node_name,
        'parentId': parent_id,
        'properties': properties,
        'tags': parse_tags(tags),
        'environment': environment
    })

    # Связь CONTAINS, если есть родитель
    if parent_id:
        relationships.append({
            'sourceId': parent_id,
            'destinationId': node_id,
            'description': 'Contains',
            'type': 'CONTAINS'
        })

    # Обрабатываем вложенные deploymentNodes (рекурсия)
    for child_node in node.get('children', []):
        process_deployment_node(child_node, elements, relationships, node_id)

    # Обрабатываем containerInstances
    for container_instance in node.get('containerInstances', []):
        ci_id = container_instance['id']
        container_id = container_instance.get('containerId')
        # Берём количество инстансов, по умолчанию 1
        instance_count = container_instance.get('instances', 1)

        elements.append({
            'id': ci_id,
            'type': 'ContainerInstance',
            'name': f"Instance of {container_id}",
            'parentId': node_id,
            'properties': container_instance.get('properties', {}),
            'tags': parse_tags(container_instance.get('tags', '')),
            'environment': container_instance.get('environment', ''),
            # Дополнительные поля:
            'containerId': container_id,
            'containerName': container_id,  # Аналогичная информация
            'instanceCount': instance_count
        })

        # Связь INSTANCE_OF между инстансом и самим контейнером
        relationships.append({
            'sourceId': ci_id,
            'destinationId': container_id,
            'description': 'InstanceOf',
            'type': 'INSTANCE_OF'
        })

        # Если у containerInstance есть свои relationships
        relationships.extend(container_instance.get('relationships', []))

    # Обрабатываем infrastructureNodes
    for infra_node in node.get('infrastructureNodes', []):
        infra_id = infra_node['id']
        elements.append({
            'id': infra_id,
            'type': 'InfrastructureNode',
            'name': infra_node['name'],
            'parentId': node_id,
            'properties': infra_node.get('properties', {}),
            'tags': parse_tags(infra_node.get('tags', '')),
            'environment': infra_node.get('environment', '')
        })
        relationships.extend(infra_node.get('relationships', []))

def import_elements(tx, elements):
    """
    Создаёт (MERGE) в графе узлы различных типов,
    в том числе ContainerInstance с полями containerName и instanceCount.
    """
    allowed_labels = [
        'Person', 'SoftwareSystem', 'Container',
        'Component', 'DeploymentNode', 'ContainerInstance',
        'InfrastructureNode'
    ]

    for element in elements:
        element_id = element.get('id')
        element_name = element.get('name')
        element_type = element.get('type')
        properties = element.get('properties', {})
        tags = element.get('tags', [])
        # technology здесь будет уже списком (после parse_technologies)
        technology = element.get('technology', [])
        environment = element.get('environment', '')
        parent_id = element.get('parentId', None)

        # Доп. поля для ContainerInstance
        container_id = element.get('containerId', None)
        container_name = element.get('containerName', None)
        instance_count = element.get('instanceCount', None)

        if element_type not in allowed_labels:
            print(f"Предупреждение: неизвестный тип узла '{element_type}'. Узел пропущен.")
            continue

        # Базовый MERGE для узла
        query = (
            f"MERGE (n:{element_type} {{id: $id}}) "
            "SET n.name = $name, n.tags = $tags, n += $properties "
        )

        params = {
            'id': element_id,
            'name': element_name,
            'tags': tags,
            'properties': properties
        }

        # Устанавливаем technology как список
        if technology:
            query += "SET n.technology = $technology "
            params['technology'] = technology

        if environment:
            query += "SET n.environment = $environment "
            params['environment'] = environment

        # Если это ContainerInstance — добавим спец. поля
        if element_type == 'ContainerInstance':
            if container_name:
                query += "SET n.containerName = $containerName "
                params['containerName'] = container_name
            if instance_count is not None:
                query += "SET n.instanceCount = $instanceCount "
                params['instanceCount'] = instance_count

        tx.run(query, **params)

        # Связь CONTAINS (родитель->ребёнок) для иерархии в Structurizr
        if parent_id:
            tx.run(
                "MATCH (parent {id: $parent_id}), (child {id: $child_id}) "
                "MERGE (parent)-[:CONTAINS]->(child)",
                parent_id=parent_id,
                child_id=element_id
            )

def import_relationships(tx, relationships):
    """
    Создаёт (MERGE) связи в графе.
    """
    allowed_rel_types = ['RELATES_TO', 'CONTAINS', 'INSTANCE_OF']

    for relationship in relationships:
        source_id = relationship.get('sourceId') or relationship.get('source')
        target_id = relationship.get('destinationId') or relationship.get('target')
        description = relationship.get('description', '')
        technology = relationship.get('technology', '')
        properties = relationship.get('properties', {})
        # Теги у связей тоже можно парсить, если нужно
        tags = parse_tags(relationship.get('tags', ''))
        rel_type = relationship.get('type', 'RELATES_TO')
        rel_id = relationship.get('id')

        if not source_id or not target_id:
            continue

        if rel_type not in allowed_rel_types:
            print(f"Предупреждение: неизвестный тип отношения '{rel_type}'. Используется 'RELATES_TO'.")
            rel_type = 'RELATES_TO'

        if rel_id:
            query = (
                "MATCH (a {id: $source_id}), (b {id: $target_id}) "
                f"MERGE (a)-[r:{rel_type} {{id: $rel_id}}]->(b) "
                "SET r.description = $description, r.technology = $technology, r.tags = $tags, r += $properties"
            )
            params = {
                'source_id': source_id,
                'target_id': target_id,
                'rel_id': rel_id,
                'description': description,
                'technology': technology,
                'properties': properties,
                'tags': tags
            }
        else:
            query = (
                "MATCH (a {id: $source_id}), (b {id: $target_id}) "
                f"MERGE (a)-[r:{rel_type}]->(b) "
                "SET r.description = $description, r.technology = $technology, r.tags = $tags, r += $properties"
            )
            params = {
                'source_id': source_id,
                'target_id': target_id,
                'description': description,
                'technology': technology,
                'properties': properties,
                'tags': tags
            }

        tx.run(query, **params)

def main():
    model = load_structurizr_model(STRUCTURIZR_JSON_PATH)
    elements, relationships = extract_elements_and_relationships(model)

    with driver.session() as session:
        session.execute_write(import_elements, elements)
        session.execute_write(import_relationships, relationships)

    print("Success")

if __name__ == "__main__":
    main()
    driver.close()