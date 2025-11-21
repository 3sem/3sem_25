# run_and_plot.py
import subprocess, shlex, os, sys
import numpy as np
import matplotlib.pyplot as plt

bins = {
    "small": 8192,      # ~ typical msg max (tune as needed)
    "medium": 65536,    # FIFO buffer ~64KB
    "large": 10048576     # >> FIFO buffer
}

programs = [
    ("SHM","./shm"),
    ("MQ","./mq"),
    ("FIFO","./fifo")
]

infile = "testfile.bin"
# prepare a test file if missing (100 MB)
if not os.path.exists(infile):
    print("Generating test file (100 MB)...")
    with open(infile,"wb") as f:
        f.write(os.urandom(100*1024*1024))

results = {k: {p[0]: None for p in programs} for k in bins}

for name, buf in bins.items():
    for label, prog in programs:
        out = f"out_{label}_{name}.bin"
        cmd = f"{prog} {infile} {out} {buf}"
        print("RUN:", cmd)
        p = subprocess.Popen(shlex.split(cmd), stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        outp, err = p.communicate()
        # parse elapsed from stdout
        elapsed = None
        for line in outp.splitlines():
            if "elapsed" in line:
                parts = line.strip().split()
                elapsed = float(parts[-1])
        if elapsed is None:
            # try to find in stderr
            for line in err.splitlines():
                if "elapsed" in line:
                    parts = line.strip().split()
                    elapsed = float(parts[-1])
        print(outp)
        results[name][label] = elapsed

# build plots: one plot per buffer size, 3 bars (SHM, MQ, FIFO)
labels = [p[0] for p in programs]
x = np.arange(len(labels))
width = 0.6

for name in bins:
    vals = [results[name][lab] for lab in labels]
    plt.figure(figsize=(6,4))
    plt.bar(x, vals, width)
    plt.xticks(x, labels)
    plt.ylabel("seconds")
    plt.title(f"Transfer time — buffer: {name} ({bins[name]} bytes)")
    for i,v in enumerate(vals):
        plt.text(i, v*1.01, f"{v:.3f}", ha='center')
    plt.tight_layout()
    plt.savefig(f"hist_{name}.png")
    print("Saved hist:", f"hist_{name}.png")

# summary print
print("RESULT TABLE")
for name in bins:
    print(name, {k:results[name][k] for k in results[name]})
