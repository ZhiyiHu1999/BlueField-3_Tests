import re
import json
import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns
import numpy as np
from matplotlib.ticker import FuncFormatter

# def log2_formatter(x, pos):
#     if x <= 0: return ""
#     exp = int(np.log2(x))
#     if 2 ** exp == x:
#         return f"2^{exp}"
#     return ""

with open("client_results.log", "r") as f:
    lines = f.read().splitlines()

results = {"bandwidth": [], "latency": []}
i = 0

while i < len(lines) - 3:
    header = lines[i].strip()
    data = lines[i + 1].strip()
    divider = lines[i + 2].strip()
    test_type = lines[i + 3].strip()

    if re.match(r"#bytes\s+#iterations", header) and re.match(r"^-+$", divider) and "ib_" in test_type:
        values = data.split()

        if "_bw" in test_type:
            results["bandwidth"].append({
                "type": test_type,
                "bytes": int(values[0]),
                "iterations": int(values[1]),
                "bw_peak": float(values[2]),
                "bw_avg": float(values[3]),
                "msg_rate": float(values[4])
            })
        elif "_lat" in test_type:
            results["latency"].append({
                "type": test_type,
                "bytes": int(values[0]),
                "iterations": int(values[1]),
                "t_min": float(values[2]),
                "t_max": float(values[3]),
                "t_typical": float(values[4]),
                "t_avg": float(values[5]),
                "t_stdev": float(values[6]),
                "t_99": float(values[7]),
                "t_999": float(values[8])
            })

        i += 4  # Skip this block
    else:
        i += 1  # Move to next line

with open("./rdma_results.json", "w") as f:
    json.dump(results, f, indent=2)

bw_df = pd.DataFrame(results["bandwidth"])
lat_df = pd.DataFrame(results["latency"])

sns.set(style="whitegrid")

for name, group in bw_df.groupby("type"):
    group = group.sort_values("bytes")
    plt.figure(figsize=(10, 6))
    plt.plot(group["bytes"], group["bw_avg"], marker='o')
    font = {'family': 'serif', 'weight': 'bold', 'size': 14}

    plt.xscale("log", base=2)
    plt.xticks(group["bytes"], [f"2^{int(np.log2(b))}" for b in group["bytes"]], fontweight="bold", fontsize=10, fontfamily="serif")
    plt.xlabel("Message Size (Bytes)", fontdict=font)
    plt.yticks(fontweight="bold", fontsize=10)
    plt.ylabel("Average Bandwidth (MiB/s)", fontdict=font)
    plt.title(f"{name} - BW Average vs Message Size", fontdict=font)
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(f"{name}_bw_avg.png")
    plt.close()

for name, group in lat_df.groupby("type"):
    group = group.sort_values("bytes")
    plt.figure(figsize=(10, 6))
    plt.plot(group["bytes"], group["t_99"], marker='o')
    font = {'family': 'serif', 'weight': 'bold', 'size': 14}

    plt.xscale("log", base=2)
    plt.xticks(group["bytes"], [f"2^{int(np.log2(b))}" for b in group["bytes"]], fontweight="bold", fontsize=10, fontfamily="serif")
    plt.xlabel("Message Size (Bytes)", fontdict=font)
    plt.yticks(fontweight="bold", fontsize=10)
    plt.ylabel("99th Percentile Latency (µs)", fontdict=font)
    plt.title(f"{name} - P99 Latency vs Message Size", fontdict=font)
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(f"{name}_p99_latency.png")
    plt.close()
