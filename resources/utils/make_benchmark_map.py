import json
import sys
import math

CHUNK_SIZE = 16

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
    "name": f"Map loading benchmark ({size}x{size} Tiles)",
    "description": "A super big map for benchmarking map loading. Made with resources/utils/make_benchmark_map.py",
    "declarations": {},
    "data": []
}

for chunk_y in range(0, size, CHUNK_SIZE):
    chunk_row = []

    for chunk_x in range(0, size, CHUNK_SIZE):
        chunk = []

        chunk_height = min(CHUNK_SIZE, size - chunk_y)
        chunk_width = min(CHUNK_SIZE, size - chunk_x)

        for tile_y in range(chunk_height):
            tile_row = []

            for tile_x in range(chunk_width):
                tile_row.append({
                    "base": "td:dirt",
                    "top": "td:top_grass",
                    "height": height,
                })

            chunk.append(tile_row)

        chunk_row.append(chunk)

    contents["data"].append(chunk_row)

with open(output_file, "w") as file:
    file.write(json.dumps(contents, indent=4))