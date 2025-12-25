# Метод Монте-Карло: Параллельное интегрирование с измерением производительности
## Описание задачи

Реализована параллельная версия метода Монте-Карло для вычисления определённого интеграла функции:

$$
f(x) = \arctan(434x + \cosh(x)) \cdot \sin(x)
$$

с использованием **многопоточности** (`pthread`) и **разделяемой памяти** (System V `shm`).

Исследуется влияние **количества потоков** на **время выполнения** и **производительность** алгоритма.

---

## Структура проекта

- `Monte_Carlo_integration.c` — основная программа.
- `generate_and_count_points.c` — функции генерации точек и подсчёта попаданий под кривую.
- `Makefile` — для запуска с разным количеством потоков.
- `results.csv` — результаты замеров времени выполнения.
- `make_log_graph.py` — скрипт для построения графиков.

---

## Запуск

### Компиляция

```bash
mkdir build && cd build
cmake ..
make
```

### Запуск с разным количеством потоков

```bash
make run_tests
```

Программа запускается с параметрами:

```bash
./Monte_Carlo_integration n_threads n_points a b m M
```

Пример:

```bash
./Monte_Carlo_integration 4 100000000 3 7 -1.6 1.5
```

---

## Результаты

<details>
<summary>Вывод команды lscpu</summary>

```bash
lscpu                                                                                                        ✔
Архитектура:                 x86_64
CPU op-mode(s):            32-bit, 64-bit
Address sizes:             46 bits physical, 48 bits virtual
Порядок байт:              Little Endian
CPU(s):                      20
On-line CPU(s) list:       0-19
ID прроизводителя:           GenuineIntel
Имя модели:                13th Gen Intel(R) Core(TM) i9-13900H
Семейство ЦПУ:           6
Модель:                  186
Thread(s) per core:      2
Ядер на сокет:           14
Сокетов:                 1
Степпинг:                2
CPU(s) scaling MHz:      17%
CPU max MHz:             5400,0000
CPU min MHz:             400,0000
BogoMIPS:                5992,00
Флаги:                   fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi
mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc art arch_perfmon
pebs bts rep_good nopl xtopology nonstop_tsc cpuid aperfmperf tsc_known_freq pni pclmulq
dq dtes64 monitor ds_cpl vmx smx est tm2 ssse3 sdbg fma cx16 xtpr pdcm pcid sse4_1 sse4_2
x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefet
ch cpuid_fault epb ssbd ibrs ibpb stibp ibrs_enhanced tpr_shadow flexpriority ept vpid ep
t_ad fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms invpcid rdseed adx smap clflushopt clwb
intel_pt sha_ni xsaveopt xsavec xgetbv1 xsaves split_lock_detect user_shstk avx_vnni dth
erm ida arat pln pts hwp hwp_notify hwp_act_window hwp_epp hwp_pkg_req hfi vnmi umip pku
ospke waitpkg gfni vaes vpclmulqdq rdpid movdiri movdir64b fsrm md_clear serialize pconfi
g arch_lbr ibt flush_l1d arch_capabilities
Virtualization features:
Виртуализация:             VT-x
Caches (sum of all):
L1d:                       544 KiB (14 instances)
L1i:                       704 KiB (14 instances)
L2:                        11,5 MiB (8 instances)
L3:                        24 MiB (1 instance)
NUMA:
NUMA node(s):              1
NUMA node0 CPU(s):         0-19
Vulnerabilities:
Gather data sampling:      Not affected
Indirect target selection: Not affected
Itlb multihit:             Not affected
L1tf:                      Not affected
Mds:                       Not affected
Meltdown:                  Not affected
Mmio stale data:           Not affected
Reg file data sampling:    Mitigation; Clear Register File
Retbleed:                  Not affected
Spec rstack overflow:      Not affected
Spec store bypass:         Mitigation; Speculative Store Bypass disabled via prctl
Spectre v1:                Mitigation; usercopy/swapgs barriers and __user pointer sanitization
Spectre v2:                Mitigation; Enhanced / Automatic IBRS; IBPB conditional; PBRSB-eIBRS SW sequence; BHI BHI
_DIS_S
Srbds:                     Not affected
Tsa:                       Not affected
Tsx async abort:           Not affected
Vmscape:                   Mitigation; IBPB before exit to userspace

```

</details>
После выполнения экспериментов данные о времени выполнения сохраняются в файл `results.csv` в формате:

```
n_threads, time_taken
```

Затем с помощью скриптов **make_sub_graph.py** и **make_log_graph.py** строятся график зависимости времени от числа потоков.

### Графики 

График показывает три фазы: 

    Гипербола — при малом числе потоков (1–16) наблюдается ускорение.
    Горизонтальная прямая (в среднем, фактически ломаная переменной монотонности из-за случайных погрешностей) — при среднем числе потоков (16–128) ускорение стабилизируется.
    Наклонная прямая вверх — при большом числе потоков (>1000) время возрастает из-за накладных расходов на синхронизацию и планирование.

![Subplots](Graphics/performance_subplots.png)
![Log_plots](Graphics/performance_logscale.png)
