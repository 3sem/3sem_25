# Task 5 - Signal-based File Transfer

Программа для передачи файлов между процессами с использованием сигналов реального времени и разделяемой памяти.

## Описание

Программа реализует механизм передачи данных между родительским и дочерним процессом через разделяемую память с синхронизацией с помощью сигналов реального времени. Производитель (родитель) читает данные из файла и передает их потребителю (дочерний процесс), который записывает данные в выходной файл.

## Сборка

### Debug сборка (с отладочной информацией и санитайзерами):
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

### Release сборка (с оптимизацией):
```bash
mkdir build && cd build  
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

## Запуск

```bash
./task5-signals <input_file> <output_file>
```

### Пример:
```bash
./task5-signals input.txt output.txt
```

## Особенности реализации

- Использует сигналы реального времени (`SIGRTMIN` - `SIGRTMIN+2`)
- Разделяемая память разбита на чанки для эффективной передачи
- Синхронизация через очередь сигналов с передачей номеров чанков

## Структура проекта

```bash
task5-signals/
├── CMakeLists.txt
├── headers/
│   ├── handlers.h
│   └── Debug_printf.h
├── src/
│   ├── main.c
│   └── handlers.c
└── build/
```

## Время передачи файла размером 4 ГБ
```bash
./task5-signals ../input_file.txt ../output_file.txt
./task5-signals: Time consumed = 4.735137 seconds
md5sum ../output_file.txt
2966ddef187d7cf4074073fc2bc0086c  ../output_file.txt
md5sum ../input_file.txt
2966ddef187d7cf4074073fc2bc0086c  ../input_file.txt
```
