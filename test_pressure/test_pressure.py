import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

plt.rcParams['figure.figsize'] = (6, 4)
plt.rcParams['savefig.dpi'] = 300

df = pd.read_csv("../bin/result.csv")
df["label"] = ["Redis", "NoRedis"]

df["qps"] = df["success"] / df["duration"]

plt.figure()
ax = plt.gca()          # 获取当前坐标轴
ax.spines[['top', 'right']].set_visible(False)
plt.bar(df["label"], df["qps"], color=["skyblue", "salmon"])
plt.ylabel("QPS")
plt.title("QPS Comparison")
for i, v in enumerate(df["qps"]):
    plt.text(i, v + 500, f"{v:.0f}", ha="center")

plt.savefig("qps_comparison.png", bbox_inches='tight')
plt.show()


latency_cols = ["avg_latency_us", "p95_latency_us", "p99_latency_us"]
x = np.arange(len(latency_cols))
width = 0.35

plt.figure()
plt.bar(x - width/2, df[latency_cols].iloc[0], width, label="Redis", color="skyblue")
plt.bar(x + width/2, df[latency_cols].iloc[1], width, label="NoRedis", color="salmon")
plt.xticks(x, latency_cols)
plt.ylabel("Latency (μs)")
plt.title("Latency Comparison")
plt.legend()
for i, (v1, v2) in enumerate(zip(df[latency_cols].iloc[0], df[latency_cols].iloc[1])):
    plt.text(i - width/2, v1 + 1000, f"{v1:.0f}", ha="center", fontsize=8)
    plt.text(i + width/2, v2 + 1000, f"{v2:.0f}", ha="center", fontsize=8)
ax = plt.gca()          # 获取当前坐标轴
ax.spines[['top', 'right']].set_visible(False)
plt.savefig("latency_comparison.png", bbox_inches='tight')
plt.show()