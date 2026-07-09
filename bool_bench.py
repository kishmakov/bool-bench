from __future__ import annotations

import atexit
import ctypes
import multiprocessing as mp
import os
import numpy as np
from collections.abc import Callable, Iterator, Sequence
from pathlib import Path
from tqdm import tqdm


_LIBRARY_NAME = "libbb.so"


def _find_library() -> Path:
    # Locate libbb.so relative to this module, so the same file works
    # whether bool-bench is checked out standalone (built into ./build) or used
    # as a submodule whose .so is built into the superproject's build dir.
    # BOOL_BENCH_LIBRARY overrides discovery with an explicit path.
    override = os.environ.get("BOOL_BENCH_LIBRARY")
    if override:
        return Path(override)
    here = Path(__file__).resolve().parent
    for base in (here, *here.parents):
        for build_dir in ("build", "cmake-build-debug"):
            candidate = base / build_dir / _LIBRARY_NAME
            if candidate.exists():
                return candidate
    return here / "build" / _LIBRARY_NAME


LIBRARY = _find_library()


def sample_point_dim(bitness: int) -> int:
    return 2 * bitness + 1


def restriction_point_dim(bitness: int) -> int:
    return sample_point_dim(bitness - 1)


def circuit_sample_point_dim(inputs: int, outputs: int) -> int:
    return inputs + outputs * (inputs + 1)


class Generator:
    def __init__(self, library_path: Path):
        self.library_path = str(library_path)
        self._library = None

    @property
    def library(self) -> ctypes.CDLL:
        if self._library is None:
            self._library = self._load_library()
        return self._library

    def _load_library(self) -> ctypes.CDLL:
        library = ctypes.CDLL(self.library_path)
        size_t_array = np.ctypeslib.ndpointer(
            dtype=np.uintp,
            ndim=1,
            flags="C_CONTIGUOUS",
        )
        float_tensor = np.ctypeslib.ndpointer(
            dtype=np.float32,
            ndim=3,
            flags="C_CONTIGUOUS",
        )

        library.bb_tree_cases_number.argtypes = [ctypes.c_uint16]
        library.bb_tree_cases_number.restype = ctypes.c_size_t

        library.bb_table_cases_number.argtypes = [ctypes.c_uint16]
        library.bb_table_cases_number.restype = ctypes.c_size_t

        library.bb_table_solvable_bitness.argtypes = []
        library.bb_table_solvable_bitness.restype = ctypes.c_uint16

        library.bb_min_tree_bitness.argtypes = []
        library.bb_min_tree_bitness.restype = ctypes.c_uint16

        library.bb_tree_nodes.argtypes = [ctypes.c_uint16, ctypes.c_size_t]
        library.bb_tree_nodes.restype = ctypes.c_size_t

        library.bb_tree_depth.argtypes = [ctypes.c_uint16, ctypes.c_size_t]
        library.bb_tree_depth.restype = ctypes.c_size_t

        library.bb_table_nodes.argtypes = [ctypes.c_uint16, ctypes.c_size_t]
        library.bb_table_nodes.restype = ctypes.c_size_t

        library.bb_table_depth.argtypes = [ctypes.c_uint16, ctypes.c_size_t]
        library.bb_table_depth.restype = ctypes.c_size_t

        library.bb_tree_value.argtypes = [
            ctypes.c_uint16,
            ctypes.c_size_t,
            ctypes.c_char_p,
        ]
        library.bb_tree_value.restype = ctypes.c_char_p

        library.bb_tree_value_tensor.argtypes = [
            ctypes.c_uint16,
            size_t_array,
            ctypes.c_size_t,
            ctypes.c_char_p,
            ctypes.c_size_t,
            float_tensor,
        ]
        library.bb_tree_value_tensor.restype = None

        library.bb_table_value.argtypes = [
            ctypes.c_uint16,
            ctypes.c_size_t,
            ctypes.c_char_p,
        ]
        library.bb_table_value.restype = ctypes.c_char_p

        library.bb_table_value_tensor.argtypes = [
            ctypes.c_uint16,
            size_t_array,
            ctypes.c_size_t,
            ctypes.c_char_p,
            ctypes.c_size_t,
            float_tensor,
        ]
        library.bb_table_value_tensor.restype = None

        library.bb_tree_restrictions.argtypes = [
            ctypes.c_uint16,
            ctypes.c_size_t,
            ctypes.c_char_p,
        ]
        library.bb_tree_restrictions.restype = ctypes.c_char_p

        library.bb_table_restrictions.argtypes = [
            ctypes.c_uint16,
            ctypes.c_size_t,
            ctypes.c_char_p,
        ]
        library.bb_table_restrictions.restype = ctypes.c_char_p

        library.bb_circuit_sets.argtypes = []
        library.bb_circuit_sets.restype = ctypes.c_char_p

        library.bb_circuit_cases.argtypes = [ctypes.c_char_p]
        library.bb_circuit_cases.restype = ctypes.c_char_p

        library.bb_circuit_inputs.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
        library.bb_circuit_inputs.restype = ctypes.c_size_t

        library.bb_circuit_outputs.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
        library.bb_circuit_outputs.restype = ctypes.c_size_t

        library.bb_circuit_value.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_char_p,
        ]
        library.bb_circuit_value.restype = ctypes.c_char_p
        return library

    def tree_cases_number(self, bitness: int) -> int:
        return int(self.library.bb_tree_cases_number(bitness))

    def min_tree_bitness(self) -> int:
        return int(self.library.bb_min_tree_bitness())

    def tree_nodes(self, bitness: int, case_id: int) -> int:
        return int(self.library.bb_tree_nodes(bitness, case_id))

    def tree_depth(self, bitness: int, case_id: int) -> int:
        return int(self.library.bb_tree_depth(bitness, case_id))

    # Result shape: 2 * bitness + 1.
    def tree_value(self, bitness: int, case_id: int, input_bits: str) -> np.ndarray:
        return self._value(self.library.bb_tree_value, bitness, case_id, input_bits)

    # Result shape: len(input_bits) x (2 * bitness + 1).
    def tree_values(
        self,
        bitness: int,
        case_id: int,
        input_bits: Sequence[str],
    ) -> np.ndarray:
        return self._values(self.library.bb_tree_value, bitness, case_id, input_bits)

    # Result shape: len(case_ids) x reps x (2 * bitness + 1).
    def tree_value_tensor(
        self,
        bitness: int,
        case_ids: Sequence[int],
        input_bits: Sequence[Sequence[str]],
    ) -> np.ndarray:
        return self._value_tensor(
            self.library.bb_tree_value_tensor,
            bitness,
            case_ids,
            input_bits,
        )

    # Result shape: (bitness * 2) x (2 * bitness - 1).
    def tree_restrictions(
        self,
        bitness: int,
        case_id: int,
        input_bits: str,
    ) -> np.ndarray:
        return self._restrictions(
            self.library.bb_tree_restrictions,
            bitness,
            case_id,
            input_bits,
        )

    def table_cases_number(self, bitness: int) -> int:
        return int(self.library.bb_table_cases_number(bitness))

    def table_solvable_bitness(self) -> int:
        return int(self.library.bb_table_solvable_bitness())

    def table_nodes(self, bitness: int, case_id: int) -> int:
        return int(self.library.bb_table_nodes(bitness, case_id))

    def table_depth(self, bitness: int, case_id: int) -> int:
        return int(self.library.bb_table_depth(bitness, case_id))

    # Result shape: 2 * bitness + 1.
    def table_value(self, bitness: int, case_id: int, input_bits: str) -> np.ndarray:
        return self._value(self.library.bb_table_value, bitness, case_id, input_bits)

    # Result shape: len(input_bits) x (2 * bitness + 1).
    def table_values(
        self,
        bitness: int,
        case_id: int,
        input_bits: Sequence[str],
    ) -> np.ndarray:
        return self._values(self.library.bb_table_value, bitness, case_id, input_bits)

    # Result shape: len(case_ids) x reps x (2 * bitness + 1).
    def table_value_tensor(
        self,
        bitness: int,
        case_ids: Sequence[int],
        input_bits: Sequence[Sequence[str]],
    ) -> np.ndarray:
        return self._value_tensor(
            self.library.bb_table_value_tensor,
            bitness,
            case_ids,
            input_bits,
        )

    # Result shape: (bitness * 2) x (2 * bitness - 1).
    def table_restrictions(
        self,
        bitness: int,
        case_id: int,
        input_bits: str,
    ) -> np.ndarray:
        return self._restrictions(
            self.library.bb_table_restrictions,
            bitness,
            case_id,
            input_bits,
        )

    def _value(
        self,
        value_fn: Callable[[int, int, bytes], bytes],
        bitness: int,
        case_id: int,
        input_bits: str,
    ) -> np.ndarray:
        value = value_fn(bitness, case_id, input_bits.encode("ascii"))
        return _ascii_bits_to_signed(value, sample_point_dim(bitness))

    def _values(
        self,
        value_fn: Callable[[int, int, bytes], bytes],
        bitness: int,
        case_id: int,
        input_bits: Sequence[str],
    ) -> np.ndarray:
        samples = np.empty(
            (len(input_bits), sample_point_dim(bitness)),
            dtype=np.float32,
        )
        for row_id, bits in enumerate(input_bits):
            samples[row_id] = self._value(value_fn, bitness, case_id, bits)
        return samples

    def _value_tensor(
        self,
        value_fn,
        bitness: int,
        case_ids: Sequence[int],
        input_bits: Sequence[Sequence[str]],
    ) -> np.ndarray:
        case_ids_array = np.ascontiguousarray(case_ids, dtype=np.uintp)
        assert case_ids_array.ndim == 1, case_ids_array.shape
        assert len(case_ids_array) == len(input_bits), (
            len(case_ids_array),
            len(input_bits),
        )
        assert input_bits, "empty input"
        reps = len(input_bits[0])
        assert all(len(case_input_bits) == reps for case_input_bits in input_bits)

        samples = np.empty(
            (len(case_ids_array), reps, sample_point_dim(bitness)),
            dtype=np.float32,
        )
        packed_inputs = _pack_input_bits(bitness, input_bits)
        value_fn(
            bitness,
            case_ids_array,
            len(case_ids_array),
            packed_inputs,
            reps,
            samples,
        )
        return samples

    def _restrictions(
        self,
        restrictions_fn: Callable[[int, int, bytes], bytes],
        bitness: int,
        case_id: int,
        input_bits: str,
    ) -> np.ndarray:
        free_bits = bitness - 1
        restrictions = bitness * 2
        assert len(input_bits) % (restrictions * free_bits) == 0
        reps = len(input_bits) // (restrictions * free_bits)
        value = restrictions_fn(bitness, case_id, input_bits.encode("ascii"))
        point_dim = restriction_point_dim(bitness)
        signed = _ascii_bits_to_signed(value, restrictions * reps * point_dim)
        return signed.reshape(restrictions, reps, point_dim)

    def circuit_sets(self) -> list[str]:
        return _split_newlines(self.library.bb_circuit_sets())

    def circuit_cases(self, set_name: str) -> list[str]:
        return _split_newlines(
            self.library.bb_circuit_cases(set_name.encode("ascii"))
        )

    def circuit_inputs(self, set_name: str, case_name: str) -> int:
        return int(self.library.bb_circuit_inputs(
            set_name.encode("ascii"),
            case_name.encode("ascii"),
        ))

    def circuit_outputs(self, set_name: str, case_name: str) -> int:
        return int(self.library.bb_circuit_outputs(
            set_name.encode("ascii"),
            case_name.encode("ascii"),
        ))

    def circuit_value(self, set_name: str, case_name: str, input_state: str) -> np.ndarray:
        value = self.library.bb_circuit_value(
            set_name.encode("ascii"),
            case_name.encode("ascii"),
            input_state.encode("ascii"),
        )
        point_dim = circuit_sample_point_dim(
            self.circuit_inputs(set_name, case_name),
            self.circuit_outputs(set_name, case_name),
        )
        return _ascii_bits_to_signed(value, point_dim)


def load_generator() -> Generator:
    return Generator(LIBRARY)


def _ascii_bits_to_signed(value: bytes, expected_len: int) -> np.ndarray:
    assert len(value) == expected_len
    bits = np.frombuffer(value, dtype=np.uint8).astype(np.int8) - ord("0")
    return bits * 2 - 1


def _pack_input_bits(bitness: int, input_bits: Sequence[Sequence[str]]) -> bytes:
    parts = []
    for case_input_bits in input_bits:
        for bits in case_input_bits:
            assert len(bits) == bitness, (len(bits), bitness)
            parts.append(bits)
    return "".join(parts).encode("ascii")


def _split_newlines(value: bytes) -> list[str]:
    text = value.decode("ascii")
    return [] if not text else text.split("\n")
