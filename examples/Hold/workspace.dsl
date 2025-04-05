workspace HOLD {
    model {
        integrationPlatform = softwareSystem  "Интеграционная Платформа" {
            description "Центральный компонент для интеграции распределённых сервисов."
        }

        distributedSystem = softwareSystem  "Распределённая система" {
            serviceA = container  "Сервис A" {
                technology "Go"
            }
            serviceB = container  "Сервис B" {
                technology "Python, Flask"
            }
            serviceC = container  "Сервис C" {
                technology "Java, Quarkus"
            }
            database = container "База данных" {
                technology "PostgreSQL"
                description "Размещена во внутренней сети (не в DMZ)."
            }
        }

        distributedSystem -> integrationPlatform "Интеграция через REST API"
    }
    views {
        systemContext distributedSystem {
            include *
            autolayout lr
        }

        systemContext integrationPlatform {
            include *
            autolayout lr
        }
    }
}
