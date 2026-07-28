import json
import sys

if len(sys.argv) in [3, 4]:
    try:
        output_file = str(sys.argv[1])
        size = int(sys.argv[2])
        if len(sys.argv) == 4:
            height = int(sys.argv[3])
        else:
            height = 4
    except:
        print(f"Invalid inputs. Usage: {sys.argv[0]} OUTPUT_FILE SIZE [HEIGHT]")
        quit()
else:
    print(f"Invalid inputs. Usage: {sys.argv[0]} OUTPUT_FILE SIZE [HEIGHT]")
    quit()

#output_file = "benchmark.jsonc"
#size = 400
#height = 4

contents = {
    "name": "Map loading benchmark",
    "description": "A super big map for benchmarking map loading. Made with resources/utils/make_benchmark_map.py",
    "declarations": {
        "textures": {
            "map:dirt_grass": [{"texture": "td:dirt", "x": 0, "y": 0}, {"texture": "td:top_grass", "x": 0, "y": 0}]
        }
    },
    "data": []
}

row = []
for i in range(size):
    row.append({"base": "td:dirt", "top_tile": "map:dirt_grass", "height": height})

for i in range(size):
    contents["data"].append(row)

with open(output_file, "w") as file:
    file.write(json.dumps(contents, indent=4))