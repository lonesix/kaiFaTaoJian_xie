import sys
import os
import json

def merge_bin():
    if os.system("idf.py merge-bin") != 0:
        print("merge bin failed")
        sys.exit(1)

def zip_bin():
    if not os.path.exists("releases"):
        os.makedirs("releases")
    output_path = f"releases/协处理器固件.zip"
    if os.path.exists(output_path):
        os.remove(output_path)
    if os.system(f"zip -j {output_path} build/merged-binary.bin") != 0:
        print("zip bin failed")
        sys.exit(1)
    print(f"zip bin to {output_path} done")

if __name__ == "__main__":
    merge_bin()

    zip_bin()