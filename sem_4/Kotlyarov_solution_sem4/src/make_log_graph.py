import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv('results.csv', header=None, names=['n_threads', 'time'])

plt.figure(figsize=(12, 6))
plt.plot(df['n_threads'], df['time'], marker='o', linestyle='-')
plt.xscale('log', base=2)
plt.title('Зависимость времени от числа потоков (логарифмическая шкала)')
plt.xlabel('Число потоков (n, log scale)')
plt.ylabel('Время выполнения (сек)')
plt.grid(True, which="both", ls="--")
plt.tight_layout()
plt.savefig('performance_logscale.png')
plt.show()
