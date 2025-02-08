MATCH (s1:SoftwareSystem)-[r]->(s2:SoftwareSystem)
WHERE s1 <> s2
AND NOT EXISTS {
    MATCH (s1)-[:INTEGRATES_WITH]->(s3:SoftwareSystem)-[:INTEGRATES_WITH]->(s2)
    WHERE "Интеграционная платформа" IN s3.tags
}
RETURN "Нарушение: прямое взаимодействие между системами (без интеграционной платформы)" AS rule,
       r AS element,
       s1, s2

-----------------------------------------------------------------------------------------------------------

MATCH (dmz:DeploymentNode {name: "DMZ"})-[:CONTAINS*]->(ci:ContainerInstance)
      -[:INSTANCE_OF]->(db:Container)
WHERE "Database" in db.tags
RETURN "Нарушение: база данных размещена в DMZ" AS rule,
       db AS element

-----------------------------------------------------------------------------------------------------------

WITH ["JS"] AS holdList
MATCH (c:Container)
WHERE ANY(tech IN c.technology WHERE tech IN holdList)
RETURN "Нарушение: использование технологии со статусом HOLD" AS rule,
       c AS container

-----------------------------------------------------------------------------------------------------------

MATCH (ci:ContainerInstance)-[:INSTANCE_OF]->(c:Container {name: "API Gateway"})
WHERE ci.instanceCount < 2
RETURN "Нарушение: контейнер MyContainer не дублирован" AS rule,
       c AS element

-----------------------------------------------------------------------------------------------------------

CALL apoc.algo.articulationPoints(
  "MATCH (cnt:Container) RETURN id(cnt) AS id",
  "MATCH (c1:Container)-[:RELATES_TO|DEPENDS_ON*1..2]-(c2:Container)
   RETURN id(c1) AS source, id(c2) AS target"
)
YIELD nodeId

MATCH (cnt:Container)
WHERE id(cnt) = nodeId

MATCH (ci:ContainerInstance)-[:INSTANCE_OF]->(cnt)

WHERE ci.instanceCount < 2

RETURN "Нарушение: контейнер является единой точкой отказа (артикуляционная точка, нет реплик)" AS rule,
       cnt AS container,
       ci  AS containerInstance
