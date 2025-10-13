import matplotlib.pyplot as plt
import numpy as np

# Данные
buffer_sizes = [8192, 16384, 65536]

shared_memory_times = [12.8, 9.3, 6.5]
message_queue_times = [10.5, 8.4, None]  # None для отсутствующего значения
fifo_times = [10.3, 7.2, 5.1]

# Преобразуем None в NaN для корректного отображения
message_queue_times_plot = [10.5, 8.4, np.nan]

plt.figure(figsize=(12, 6))

x = np.arange(len(buffer_sizes))
width = 0.25

plt.bar(x - width, shared_memory_times, width, label='Shared Memory', color='skyblue', edgecolor='navy')
plt.bar(x, message_queue_times_plot, width, label='Message Queue', color='lightcoral', edgecolor='darkred')
plt.bar(x + width, fifo_times, width, label='FIFO Channel', color='lightgreen', edgecolor='darkgreen')

plt.xlabel('Размер буфера (байты)')
plt.ylabel('Время (секунды)')
plt.title('Сравнение времени передачи файла через разные методы IPC', fontweight='bold')
plt.xticks(x, ['8K', '16K', '64K'])
plt.legend()
plt.grid(axis='y', alpha=0.3)

# Добавляем значения на столбцы
for i, v in enumerate(shared_memory_times):
    plt.text(i - width, v + 0.1, f'{v}', ha='center', va='bottom', fontsize=9)
for i, v in enumerate(message_queue_times):
    if v is not None:
        plt.text(i, v + 0.1, f'{v}', ha='center', va='bottom', fontsize=9)
for i, v in enumerate(fifo_times):
    plt.text(i + width, v + 0.1, f'{v}', ha='center', va='bottom', fontsize=9)

plt.tight_layout()
plt.show()
