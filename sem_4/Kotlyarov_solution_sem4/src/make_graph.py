import pandas as pd
import matplotlib.pyplot as plt

# Читаем CSV
df = pd.read_csv('results.csv', header=None, names=['n_threads', 'time'])

# Разделяем на сегменты
df_small = df[df['n_threads'] <= 16]
df_medium = df[(df['n_threads'] > 16) & (df['n_threads'] <= 128)]
df_large = df[df['n_threads'] > 1000]  # или другое пороговое значение

# Создаём 3 графика
fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(18, 5))

# График 1: Малое количество потоков (гипербола)
ax1.plot(df_small['n_threads'], df_small['time'], marker='o', label='n <= 16', color='blue')
ax1.set_title('Малое количество потоков (гипербола)')
ax1.set_xlabel('Число потоков (n)')
ax1.set_ylabel('Время (сек)')
ax1.grid(True)

# График 2: Среднее количество потоков (горизонтальная прямая)
ax2.plot(df_medium['n_threads'], df_medium['time'], marker='s', label='16 < n <= 128', color='green')
ax2.set_title('Среднее количество потоков (стагнация)')
ax2.set_xlabel('Число потоков (n)')
ax2.set_ylabel('Время (сек)')
ax2.grid(True)

# График 3: Большое количество потоков (наклонная прямая)
if not df_large.empty:
    ax3.plot(df_large['n_threads'], df_large['time'], marker='^', label='n > 1000', color='red')
    ax3.set_title('Большое количество потоков (накладные расходы)')
    ax3.set_xlabel('Число потоков (n)')
    ax3.set_ylabel('Время (сек)')
    ax3.grid(True)
else:
    ax3.text(0.5, 0.5, 'Нет данных для n > 1000', ha='center', va='center', transform=ax3.transAxes)
    ax3.set_title('Большое количество потоков (нет данных)')

plt.tight_layout()
plt.savefig('performance_plot_split.png')
plt.show()
