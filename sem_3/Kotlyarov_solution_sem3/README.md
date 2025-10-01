# IPC Performance Comparison

## Описание проекта

Проект содержит реализации трех методов межпроцессного взаимодействия (IPC) для передачи данных между процессами:
- FIFO (Named Pipes)
- System V Message Queues
- System V Shared Memory

## Структура проекта

### Исполняемые файлы

- `FIFO_transmission_main` - тестирование передачи данных через FIFO
- `SYS_V_mq_main` - тестирование передачи данных через System V Message Queues
- `SYS_V_shm_main` - тестирование передачи данных через System V Shared Memory
- `Test_main` - основной тестовый скрипт для сравнения всех методов

### Сборка проекта

```bash
# Создание директории сборки:
mkdir build && cd build

# Для Debug сборки (по умолчанию):
cmake ..
cmake --build .

# Для Release сборки:
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Для сборки с санитайзером:
cmake .. -DUSE_SANITIZER=ON
cmake --build .
```

### Запуск программ

```bash
./FIFO_transmission_main <input_file> > [output_file]
./SYS_V_mq_main <input_file> > [output_file]
./SYS_V_shm_main <input_file> > [output_file]
./Test_main <input_file_1> <input_file_2> <input_file_3> [output_file]
```

После выполнения тестов генерируются PNG файлы с гистограммами сравнения времени передачи для каждого размера файла.

## Результаты производительности

На основе тестов с файлами различного размера (8196 байт, 4 Мб, 4 Гб):

    System V Shared Memory (SHM) показывает наилучшую производительность на маленьких файлах за счет того, что не требуется лишнее копирование во временный буфер. Однако на больших файлах сильно утсупает FIFO и MQ из-за дополнительных действий, связанных с синхронизацией.
    System V Message Queues (MQ) демонстрирует наименьшее время передачи на больших файлах.
    FIFO показал средние результаты во всех тестах.

# Вывод
Копирование куда менее страшный расход времени, чем синхронизация.
