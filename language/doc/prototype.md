
## Основные конструции языка

Основной конструцией является правило:
```
rule "<name>" {

}
```

> "Похоже на drools"

У поля могут быть следущие параметры:
```
1. description: "<text>" - описание правила
2. priority: Info|Warn|Error - по умолчанию ERROR
```

Далее идут следующие основные сущности языка:
1. system

2. container

3. component

4. code

5. deploy

6. infrastructure
Это множества, содержащие все такие компоненты в модели.
чтобы выбрать элемент из этого множества можно написать следующее

```

s in system

s1, s2 in system // s1 != s2

```
Можно добавить модификаторы общности
```

all {

    s in system

}

  

exist {

    s in system

}

```
Запросы могут быть вложенными
```

all {

    s in system: exist { c in s: ... }

}

```
Как это работает. Все объекты связанные с работой над основными сущностями являются по сути множествами. И модификаторы ограничивают эти множества
После двоеточия идет предикат. Также на одном уровне можно делать выборку из разных слоев C4
Описание предикатов:
```
7. if <expr> then <expr> else <expr> - тернарный оператор
8. <item>.<prop> is number|string|list|none - если не существует такого свойства, то ничего
9. <item>.!<prop> - если не сущестует такого свойства, то нарушение правила
10. <item>.<prop> in list - посмотреть, что элемент в списке
11. булевые операторы - not, and, or, xor, >, <, >=, <=, ==, /=
```

Особые операторы:
```
12. count( set ) - список элементов в листе, списке
13. route( <item1>, <item2> ) - проверка, что связь существует между двумя списками
14. <item> in route( <item1>, <item2> )- связь осуществляется через такой-то узел
15. all{ route(<item1>, <item2>) } - все связи существуют
16. not route() - не существуют
17. cross(set)
18. union(set)
```
``except - исключение из правила``

  

### Примеры правил

  

```
rule Platform {
    description: "Test";
    priority: Error;
    all {
        s1, s2 in system:
        all {
            s in route(s1, s2): "Integration platform" in s.tags
        }
    };
    except all {
        s1, s2 in system:
            "Integration platform" in s1.tags xor "Integration platform" in s2.tags
    }
}
```

  

```
rule "DMZ" {
    all {
        d in deploy:
            "DMZ" == deploy.name:
             all { c in d:
	         "Database" in c.tags and c == none
        } 
    }
}
```

  

```
rule "HOLD" {
    lst = ["JS"];
    all {
        c in containers:
            cross(c.tech, lst) == none
    }
}
```