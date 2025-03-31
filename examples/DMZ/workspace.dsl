workspace {
    model {
        integrationPlatform = softwareSystem "Интеграционная Платформа" {
            description "Центральный компонент для связи микросервисов."
        }

        s2 = softwareSystem "Система Микросервисов" {
            serviceA = container "Сервис A" {
                technology "Java, Spring Boot"
                description "Использует проверенные технологии (не HOLD)."
            }
            serviceB = container serviceB "Сервис B" {
                technology "Node.js"
                description "Использует проверенные технологии (не HOLD)."
            }
            serviceC = container "Сервис C" {
                technology "Go"
                description "Использует проверенные технологии (не HOLD)."
            }
            db = container "База данных" {
                technology "PostgreSQL"
                description "Размещена во внутренней сети (не в DMZ)."
                tags "Database"
            }

            serviceA -> serviceB
            serviceB -> serviceC
            serviceC -> db
        }

        s2 -> integrationPlatform "Интеграция через REST API"

        deploymentEnvironment "Production" {
            deploymentNode "DMZ" {
                deploymentNode "Database deploy" {
                    containerInstance db
                    instances 2
                }
            }
        }
    }
     
    
}