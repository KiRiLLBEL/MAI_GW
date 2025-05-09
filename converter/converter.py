import os
import argparse
import json
from neo4j import GraphDatabase


def parse_args():
    parser = argparse.ArgumentParser(
        description="Импорт модели Structurizr в Neo4j")
    parser.add_argument(
        "-f", "--file",
        required=True,
        help="Путь к JSON файлу модели (например, workspace.json)"
    )
    parser.add_argument(
        "--neo4j_uri",
        default=os.getenv("NEO4J_URI", "bolt://localhost:7687"),
        help="URI подключения к Neo4j (по умолчанию берётся из переменной окружения NEO4J_URI или используется 'bolt://localhost:7687')"
    )
    parser.add_argument(
        "--neo4j_user",
        default=os.getenv("NEO4J_USER", "USERNAME"),
        help="Имя пользователя для подключения к Neo4j (по умолчанию берётся из переменной окружения NEO4J_USER или используется 'USERNAME')"
    )
    parser.add_argument(
        "--neo4j_password",
        default=os.getenv("NEO4J_PASSWORD", "PASSWORD"),
        help="Пароль для подключения к Neo4j (по умолчанию берётся из переменной окружения NEO4J_PASSWORD или используется 'PASSWORD')"
    )
    return parser.parse_args()


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

    for person in model['model'].get('people', []):
        elements.append({
            'id': person['id'],
            'type': 'Person',
            'name': person['name'],
            'properties': person.get('properties', {}),
            'tags': parse_tags(person.get('tags', ''))
        })
        relationships.extend(person.get('relationships', []))

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

    for deployment_node in model['model'].get('deploymentNodes', []):
        process_deployment_node(deployment_node, elements, relationships)

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

    elements.append({
        'id': node_id,
        'type': node_type,
        'name': node_name,
        'parentId': parent_id,
        'properties': properties,
        'tags': parse_tags(tags),
        'environment': environment
    })

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
        instance_count = container_instance.get('instances', 1)

        elements.append({
            'id': ci_id,
            'type': 'ContainerInstance',
            'name': f"Instance of {container_id}",
            'parentId': node_id,
            'properties': container_instance.get('properties', {}),
            'tags': parse_tags(container_instance.get('tags', '')),
            'environment': container_instance.get('environment', ''),
            'containerId': container_id,
            'containerName': container_id,
            'instanceCount': instance_count
        })

        relationships.append({
            'sourceId': ci_id,
            'destinationId': container_id,
            'description': 'InstanceOf',
            'type': 'INSTANCE_OF'
        })

        relationships.extend(container_instance.get('relationships', []))

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
        technology = element.get('technology', [])
        environment = element.get('environment', '')
        parent_id = element.get('parentId', None)
        container_id = element.get('containerId', None)
        container_name = element.get('containerName', None)
        instance_count = element.get('instanceCount', None)

        if element_type not in allowed_labels:
            print(
                f"Предупреждение: неизвестный тип узла '{element_type}'. Узел пропущен.")
            continue

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

        if technology:
            query += "SET n.technology = $technology "
            params['technology'] = technology

        if environment:
            query += "SET n.environment = $environment "
            params['environment'] = environment

        if element_type == 'ContainerInstance':
            if container_name:
                query += "SET n.containerName = $containerName "
                params['containerName'] = container_name
            if instance_count is not None:
                query += "SET n.instanceCount = $instanceCount "
                params['instanceCount'] = instance_count

        tx.run(query, **params)

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
        target_id = relationship.get(
            'destinationId') or relationship.get('target')
        description = relationship.get('description', '')
        technology = relationship.get('technology', '')
        properties = relationship.get('properties', {})
        tags = parse_tags(relationship.get('tags', ''))
        rel_type = relationship.get('type', 'RELATES_TO')
        rel_id = relationship.get('id')

        if not source_id or not target_id:
            continue

        if rel_type not in allowed_rel_types:
            print(
                f"Предупреждение: неизвестный тип отношения '{rel_type}'. Используется 'RELATES_TO'.")
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


def create_graph_projection(tx):
    """
    Создаёт проекцию графа для использования в GDS и запускает алгоритм поиска точек сочленения.
    Если проекция с заданным именем уже существует, она будет удалена.
    """
    # Удаляем существующую проекцию, если она есть
    tx.run("CALL gds.graph.drop('Graph', false) YIELD graphName")

    projection_query = """
    CALL gds.graph.project(
      'Graph',
      ['Person', 'SoftwareSystem', 'Container', 'Component', 'DeploymentNode', 'ContainerInstance', 'InfrastructureNode'],
      {
        RELATES_TO: {
          type: 'RELATES_TO',
          orientation: 'UNDIRECTED'
        },
        CONTAINS: {
          type: 'CONTAINS',
          orientation: 'UNDIRECTED'
        },
        INSTANCE_OF: {
          type: 'INSTANCE_OF',
          orientation: 'UNDIRECTED'
        }
      }
    ) YIELD graphName, nodeCount, relationshipCount
    WITH graphName
    CALL gds.articulationPoints.write(graphName, { writeProperty: 'articulationPoint'})
    YIELD articulationPointCount
    RETURN articulationPointCount
    """
    tx.run(projection_query)


def main():
    args = parse_args()

    driver = GraphDatabase.driver(
        args.neo4j_uri,
        auth=(args.neo4j_user, args.neo4j_password)
    )

    model = load_structurizr_model(args.file)
    elements, relationships = extract_elements_and_relationships(model)

    with driver.session() as session:
        session.execute_write(import_elements, elements)
        session.execute_write(import_relationships, relationships)
        session.execute_write(create_graph_projection)

    driver.close()
    print("Success")


if __name__ == "__main__":
    main()
