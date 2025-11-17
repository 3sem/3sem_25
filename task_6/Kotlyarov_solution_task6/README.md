# Memory Monitor Daemon

Мониторинг изменений в памяти процессов Linux в реальном времени. Демон отслеживает изменения в memory mappings целевого процесса и создает бэкапы.

## Сборка проекта

### Требования
- CMake 3.16+
- GCC с поддержкой C11
- Linux (требуется доступ к /proc/ файловой системе)

### Сборка
```bash
# Создание директории для сборки
mkdir build && cd build

# Конфигурация CMake
cmake ..

# Сборка
make
```

```bash
# Или сборка в режиме Release
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

### Структура проекта
```
Kotlyarov_solution_task6/
├── CMakeLists.txt
├── headers/           # Заголовочные файлы
├── libs/             # Вспомогательные библиотеки
│   └── Dynamic_array/
├── src/              # Исходный код
└── build/            # Директория сборки
```

## Использование

### Запуск в интерактивном режиме
```bash
./daemon -p <PID> -i
```
Пример:
```bash
./daemon -p 1234 -i
```

### Запуск в режиме демона
```bash
./daemon -p <PID> -d [-T <interval_in_microseconds>]
```
Примеры:
```bash
# С интервалом по умолчанию
./daemon -p 1234 -d

# С пользовательским интервалом (1 секунда)
./daemon -p 1234 -d -T 1000000
```

### Параметры командной строки
- `-p <PID>`: ID целевого процесса (обязательный)
- `-i`: Интерактивный режим
- `-d`: Режим демона
- `-T <us>`: Интервал опроса в микросекундах (по умолчанию: 2000000)

## Управление демоном

### Сигналы
- `SIGTERM` / `SIGINT`: Корректное завершение демона
- `SIGHUP`: Перезагрузка конфигурации (перечитывает memory maps)
- `SIGUSR1`: Создание ручного бэкапа

### Команды через FIFO
Демон создает named pipe для приема команд:
```bash
echo "command" > logs/daemon_fifo
```

### Доступные команды
- `status` - текущий статус
- `interval <us>` - изменить интервал опроса
- `pid <new_pid>` - сменить целевой PID
- `backup` - создать ручной бэкап
- `reload` - перезагрузить конфигурацию
- `help` - справка по командам
- `quit` / `stop` - остановить демон

## Примеры использования

### Запуск мониторинга веб-сервера
```bash
# Запустить демон
./daemon -p 1234 -d -T 500000

# Отправить команду статуса
echo "status" > logs/daemon_fifo

# Создать ручной бэкап
echo "backup" > logs/daemon_fifo
```

### Интерактивное управление
```bash
./daemon -p 1234 -i

>>> status
Current PID: 1234
Sample interval: 2000000 us
Status: running

>>> interval 1000000
Interval changed to: 1000000 us

>>> backup
Manual backup created

>>> quit
Exiting...
```

## Структура бэкапов

### Полные бэкапы
`backups/full/backup_<PID>_<timestamp>.maps`
- Полный снимок memory mappings процесса

### Инкрементальные бэкапы
`backups/incremental/diff_<PID>_<timestamp>.diff`
- Разница между предыдущим и текущим снимком
- Содержит добавленные, удаленные и измененные регионы

## Логирование

### Основной лог
`logs/monitor.log` - основные события приложения

### Лог демона
`logs/daemon_output.log` - вывод демонизированного процесса

### Просмотр логов в реальном времени
```bash
tail -f logs/monitor.log
tail -f logs/daemon_output.log
```

## Диагностика

### Проверка работы демона
```bash
# Поиск процесса демона
ps aux | grep daemon

# Проверка логов
tail -n 20 logs/monitor.log

# Проверка созданных бэкапов
ls -la backups/full/
ls -la backups/incremental/
```

### Возможные проблемы
1. **Нет доступа к /proc/<PID>/maps**
   - Проверьте существование процесса
   - Убедитесь в правах доступа

2. **Демон не создает бэкапы**
   - Проверьте директорию logs/
   - Убедитесь, что процесс имеет права на запись

3. **Ошибки сегментации**
   - Пересоберите с `-fsanitize=address`
   - Проверьте логи на наличие ошибок

## Разработка

### Структура кода
- `main.c` - точка входа, парсинг аргументов
- `monitor_modes.c` - интерактивный и демон режимы
- `parse_maps.c` - чтение memory maps из /proc/
- `maps_diff.c` - сравнение снимков памяти
- `backup_funcs.c` - создание бэкапов
- `command_handler.c` - обработка команд
- `parse_args.c` - парсинг аргументов CLI

### Сборка для разработки
```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```
