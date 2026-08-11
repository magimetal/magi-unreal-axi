#!/usr/bin/env python3
"""Generate Magi P1.5's original, deterministic glTF animation seed."""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path


def generate() -> bytes:
    binary = bytearray()
    buffer_views: list[dict[str, object]] = []
    accessors: list[dict[str, object]] = []

    def align4() -> None:
        while len(binary) % 4:
            binary.append(0)

    def add_blob(data: bytes, target: int | None = None) -> int:
        align4()
        offset = len(binary)
        binary.extend(data)
        view: dict[str, object] = {
            "buffer": 0,
            "byteOffset": offset,
            "byteLength": len(data),
        }
        if target is not None:
            view["target"] = target
        buffer_views.append(view)
        return len(buffer_views) - 1

    def add_accessor(
        view: int,
        component_type: int,
        value_type: str,
        count: int,
        *,
        minimum: list[float | int] | None = None,
        maximum: list[float | int] | None = None,
    ) -> int:
        accessor: dict[str, object] = {
            "bufferView": view,
            "componentType": component_type,
            "count": count,
            "type": value_type,
        }
        if minimum is not None:
            accessor["min"] = minimum
        if maximum is not None:
            accessor["max"] = maximum
        accessors.append(accessor)
        return len(accessors) - 1

    # Original low-poly vertical plane. One joint owns every vertex.
    positions = [
        (-0.25, 0.0, 0.0),
        (0.25, 0.0, 0.0),
        (0.25, 0.0, 1.0),
        (-0.25, 0.0, 1.0),
    ]
    normals = [(0.0, -1.0, 0.0)] * len(positions)
    indices = [0, 2, 1, 0, 3, 2]
    position_accessor = add_accessor(
        add_blob(b"".join(struct.pack("<3f", *value) for value in positions), 34962),
        5126,
        "VEC3",
        len(positions),
        minimum=[-0.25, 0.0, 0.0],
        maximum=[0.25, 0.0, 1.0],
    )
    normal_accessor = add_accessor(
        add_blob(b"".join(struct.pack("<3f", *value) for value in normals), 34962),
        5126,
        "VEC3",
        len(normals),
        minimum=[0.0, -1.0, 0.0],
        maximum=[0.0, -1.0, 0.0],
    )
    index_accessor = add_accessor(
        add_blob(b"".join(struct.pack("<H", value) for value in indices), 34963),
        5123,
        "SCALAR",
        len(indices),
        minimum=[0],
        maximum=[3],
    )
    joint_accessor = add_accessor(
        add_blob(b"".join(struct.pack("<4H", 0, 0, 0, 0) for _ in positions), 34962),
        5123,
        "VEC4",
        len(positions),
        minimum=[0, 0, 0, 0],
        maximum=[0, 0, 0, 0],
    )
    weight_accessor = add_accessor(
        add_blob(b"".join(struct.pack("<4f", 1.0, 0.0, 0.0, 0.0) for _ in positions), 34962),
        5126,
        "VEC4",
        len(positions),
        minimum=[1.0, 0.0, 0.0, 0.0],
        maximum=[1.0, 0.0, 0.0, 0.0],
    )
    inverse_bind_accessor = add_accessor(
        add_blob(struct.pack("<16f", 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)),
        5126,
        "MAT4",
        1,
    )

    idle_times = [0.0, 1.0, 2.0]
    idle_rotations = [(0.0, 0.0, 0.0, 1.0)] * len(idle_times)
    moving_times = [0.0, 0.5, 1.0, 1.5]
    moving_rotations = [
        (0.0, 0.0, 0.0, 1.0),
        (0.0, 0.0, 0.1736481777, 0.9848077530),
        (0.0, 0.0, -0.1736481777, 0.9848077530),
        (0.0, 0.0, 0.0, 1.0),
    ]

    def animation(name: str, times: list[float], rotations: list[tuple[float, ...]]) -> dict[str, object]:
        time_accessor = add_accessor(
            add_blob(b"".join(struct.pack("<f", value) for value in times)),
            5126,
            "SCALAR",
            len(times),
            minimum=[min(times)],
            maximum=[max(times)],
        )
        rotation_accessor = add_accessor(
            add_blob(b"".join(struct.pack("<4f", *value) for value in rotations)),
            5126,
            "VEC4",
            len(rotations),
        )
        return {
            "name": name,
            "samplers": [{"input": time_accessor, "interpolation": "LINEAR", "output": rotation_accessor}],
            "channels": [{"sampler": 0, "target": {"node": 1, "path": "rotation"}}],
        }

    gltf = {
        "asset": {"version": "2.0", "generator": "magi-unreal-axi-p15-seed/1"},
        "scene": 0,
        "scenes": [{"name": "MagiP15SeedScene", "nodes": [0]}],
        "nodes": [
            {"name": "MagiP15SeedMeshNode", "mesh": 0, "skin": 0, "children": [1]},
            {"name": "RootBone"},
        ],
        "meshes": [{
            "name": "MagiP15SeedMesh",
            "primitives": [{
                "attributes": {
                    "POSITION": position_accessor,
                    "NORMAL": normal_accessor,
                    "JOINTS_0": joint_accessor,
                    "WEIGHTS_0": weight_accessor,
                },
                "indices": index_accessor,
                "mode": 4,
                "material": 0,
            }],
        }],
        "materials": [{
            "name": "MagiP15SeedMaterial",
            "pbrMetallicRoughness": {
                "baseColorFactor": [0.15, 0.65, 1.0, 1.0],
                "metallicFactor": 0.0,
                "roughnessFactor": 1.0,
            },
            "doubleSided": True,
        }],
        "skins": [{
            "name": "MagiP15SeedSkin",
            "inverseBindMatrices": inverse_bind_accessor,
            "joints": [1],
            "skeleton": 1,
        }],
        "animations": [
            animation("Idle", idle_times, idle_rotations),
            animation("Moving", moving_times, moving_rotations),
        ],
        "buffers": [{"byteLength": len(binary)}],
        "bufferViews": buffer_views,
        "accessors": accessors,
    }
    json_bytes = json.dumps(gltf, sort_keys=True, separators=(",", ":")).encode()
    json_bytes += b" " * ((4 - len(json_bytes) % 4) % 4)
    binary_bytes = bytes(binary) + b"\0" * ((4 - len(binary) % 4) % 4)
    length = 12 + 8 + len(json_bytes) + 8 + len(binary_bytes)
    return (
        struct.pack("<4sII", b"glTF", 2, length)
        + struct.pack("<I4s", len(json_bytes), b"JSON")
        + json_bytes
        + struct.pack("<I4s", len(binary_bytes), b"BIN\0")
        + binary_bytes
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    payload = generate()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(payload)
    print(f"size={len(payload)}")
    print(f"sha256={hashlib.sha256(payload).hexdigest()}")


if __name__ == "__main__":
    main()
