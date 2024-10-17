workspace {
    name "имя продукта"
    description "описание продукта"

    # включаем режим с иерархической системой идентификаторов
    !identifiers hierarchical

    model {

        u1 = person "Студент"
        u2 = person "Преподаватель"

        s1 = softwareSystem "BigBlueButton"
        s2 = softwareSystem "LMS" {
            -> s1 "Запуск конференции в bigBlueButton"

            db = container "Database" {
                technology "PostgreSQL 14"
            }

            subject = container "Subject" {
                technology "Java Spring"
                -> db "сохранение и получение информации о курсе" "JDBC"
            }

            content = container "Content" {
                technology "Java Spring"
                -> db "сохранение и получение информации о контенте курса" "JDBC"
            }

            be = container "API Gateway" {
                -> subject "Создание/удаление курса" "HTTPS"
                -> content "Создание/удаление контента" "HTTPS"
                -> s1 "Запустить конференцию" "HTTPS"
                technology "Java Spring Cloud Gateway"

                c1 = component "Single Sign On" {

                }
                
                c2 = component "Logging Adpater" {

                }
            }
            fe = container "Single Page Application" {
                technology "JS, React"
                -> be "Получение/изменение контента курсов" "HTTPS"
            }
        }

        u1 -> s2.fe "Скачать материалы/задания и закачать результаты практики"
        u2 -> s2.fe "Загрузить материалы/задания и оценить результаты практики"


        u1 -> s2 "Получает задание, смотрит материалы, отправляет лр"
        u2 -> s2 "Создает контент курса, проверяет задания студентов"
        u1 -> s1 "слушает лекцию" {
            tags "video"
        }

        u2 -> s1 "читает лекцию" {
            tags "video"
        }


        deploymentEnvironment "Production" {

            deploymentNode "DMZ" {
                deploymentNode "NGinx Server" {
                    containerInstance s2.fe
                    instances 2
                }
            }

            deploymentNode "Inside" {

                in_db = infrastructureNode "Backup Database Server" 
                dn_db = deploymentNode "Database Server" {
                 containerInstance s2.db
                 -> in_db "Backup"
                }

                deploymentNode "k8s pod backend" {
                 containerInstance s2.be
                 instances 3
                }

                deploymentNode "k8s pod subject" {
                    containerInstance s2.subject
                }

                deploymentNode "k8s pod content" {
                    containerInstance s2.content
                }

            }

        }
        
    }

    views {

        dynamic s2 "uc01" "получение задания лабораторной работы" {
            autoLayout lr

            u1 -> s2.fe "Открыть страницу курса"
            s2.fe -> s2.be "GET /subject/{id}"
            s2.be -> s2.subject "GET /subject/{id}"
            s2.subject -> s2.db "SELECT * FROM subject WHERE id={id}"

            u1 -> s2.fe "Загрузить ЛР"
            s2.fe -> s2.be "GET /content/{id}"
            s2.be -> s2.content "GET /content/{id}"
            s2.content -> s2.db "SELECT * FROM content WHERE id={id}"
        }

        dynamic s2 "uc02" "загрузить решение лабораторной работы" {
            autoLayout lr

            u1 -> s2.fe "Открыть страницу курса"
            s2.fe -> s2.be "GET /subject/{id}"
            s2.be -> s2.subject "GET /subject/{id}"
            s2.subject -> s2.db "SELECT * FROM subject WHERE id={id}"

            u1 -> s2.fe "Загрузить решение ЛР"
            s2.fe -> s2.be "POST /content/{id}"
            s2.be -> s2.content "POST /content/{id}"
            s2.content -> s2.db "INSERT INTO  content (...) VALUES (...)"
        }

        themes default
        systemContext s2 {
            include *
            exclude relationship.tag==video
            autoLayout
        }

        container s2 "vertical" {
            include *
            autoLayout
        }

        container s2 "hotizontal" {
            include *
            autoLayout lr
        }

        component s2.be {
            include *
            autoLayout lr
        }

        deployment * "Production" {
            include *
            autoLayout

        }


    }
}