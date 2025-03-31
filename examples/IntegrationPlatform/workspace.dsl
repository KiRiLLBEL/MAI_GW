workspace {
    model {
        integrationPlatform = softwareSystem  "Интеграционная Платформа" {
            description "Централизованная платформа для интеграции всех систем."
            tags "Integration platform"
        }

        systemA = softwareSystem "Система A" {
            container webApp "Веб-приложение" {
                technology "Java, Spring Boot"
            }
            container databasePrimary "Основная БД" {
                technology "Oracle"
                description "Размещена во внутренней сети (не в DMZ)."
            }
            container databaseReplica "Резервная БД" {
                technology "Oracle"
                description "Резервный экземпляр для отказоустойчивости."
            }
        }

        systemB = softwareSystem "Система B" {
            container api "API-сервис" {
                technology "Node.js"
            }
            container databasePrimary "Основная БД" {
                technology "SQL Server"
                description "Размещена во внутренней сети (не в DMZ)."
            }
            container databaseReplica "Резервная БД" {
                technology "SQL Server"
                description "Резервный экземпляр для отказоустойчивости."
            }
        }

        systemC = softwareSystem  "Система C" {
            container mobileApp "Мобильное приложение" {
                technology "Kotlin/Swift"
            }
            container databasePrimary "Основная БД" {
                technology "PostgreSQL"
                description "Размещена во внутренней сети (не в DMZ)."
            }
            container databaseReplica "Резервная БД" {
                technology "PostgreSQL"
                description "Резервный экземпляр для отказоустойчивости."
            }
        }

        systemA -> integrationPlatform "Интеграция через SOAP/REST"
        systemB -> integrationPlatform "Интеграция через SOAP/REST"
        systemC -> integrationPlatform "Интеграция через SOAP/REST"
        systemC -> systemA "Нарушение"
    }
    views {
        systemContext integrationPlatform {
            include *
            autolayout lr
        }
    }
}
