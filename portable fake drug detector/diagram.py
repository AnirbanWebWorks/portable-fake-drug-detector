import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# Load Excel file
df = pd.read_excel("ML training data.xlsx")

# Extract Vitamin C data
row = df[df["sample label"] == "50ml water + 500mg Vitamin C"].iloc[0]
values = np.array([float(x) for x in row["data values"].split(",") if x.strip()])

# Plot electrochemical signature
plt.figure(figsize=(6,4))
plt.plot(values, linewidth=2)

# Annotate peak current
peak_index = np.argmax(values)
peak_value = np.max(values)

plt.annotate(
    "Peak Current",
    xy=(peak_index, peak_value),
    xytext=(peak_index - 12, peak_value - 400),
    arrowprops=dict(arrowstyle="->", linewidth=1)
)

plt.xlabel("Potential Scan Index")
plt.ylabel("Current (a.u.)")
plt.tight_layout()

# Save figure
plt.savefig("Fig6_Electrochemical_Signature.png", dpi=300)
plt.savefig("Fig6_Electrochemical_Signature.svg")
plt.show()
