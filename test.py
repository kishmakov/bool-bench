import ctypes
from pathlib import Path


LIBRARY = Path(__file__).resolve().parent / "build" / "libgenerator.so"

GOLDEN_VALUES = {
    (2, 3, "01"): "01001",
    (2, 15, "01"): "01111",
    (3, 31, "101"): "1010101",
    (3, 188, "110"): "1101101",
    (4, 3190, "0001"): "000100100",
    (4, 11304, "1101"): "110111001",
    (5, 3261348405, "11000"): "11000100011",
    (5, 390455940, "01001"): "01001010000",
    (6, 2547012052, "110111"): "1101110110110",
    (6, 883941716, "011111"): "0111110100001",
}


def load_library():
    library = ctypes.CDLL(str(LIBRARY))

    library.bb_get_cases_number.argtypes = [ctypes.c_uint16]
    library.bb_get_cases_number.restype = ctypes.c_size_t

    library.bb_case_depth.argtypes = [ctypes.c_uint16, ctypes.c_size_t]
    library.bb_case_depth.restype = ctypes.c_size_t

    library.bb_case_value.argtypes = [
        ctypes.c_uint16,
        ctypes.c_size_t,
        ctypes.c_char_p,
    ]
    library.bb_case_value.restype = ctypes.c_char_p

    library.bb_case_restrictions.argtypes = [
        ctypes.c_uint16,
        ctypes.c_size_t,
        ctypes.c_size_t,
    ]
    library.bb_case_restrictions.restype = ctypes.c_char_p

    library.bb_parity_value.argtypes = [
        ctypes.c_uint16,
        ctypes.c_char_p,
    ]
    library.bb_parity_value.restype = ctypes.c_char_p

    library.bb_parity_restrictions.argtypes = [
        ctypes.c_uint16,
        ctypes.c_size_t,
    ]
    library.bb_parity_restrictions.restype = ctypes.c_char_p

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


def split_list(value):
    text = value.decode("ascii")
    return [] if not text else text.split("\n")


def circuit_value(library, set_name, case_name, input_state):
    return library.bb_circuit_value(
        set_name.encode("ascii"),
        case_name.encode("ascii"),
        input_state.encode("ascii"),
    ).decode("ascii")


def case_value(library, bitness, case_id, input_bits):
    return library.bb_case_value(
        bitness,
        case_id,
        input_bits.encode("ascii"),
    ).decode("ascii")


def test_case_value(library):
    print(f"Check bb_case_value ...")

    for (bitness, case_id, input_bits), expected in GOLDEN_VALUES.items():
        assert case_id < library.bb_get_cases_number(bitness)
        assert case_value(library, bitness, case_id, input_bits) == expected


def test_case_restrictions(library):
    print(f"Check bb_case_restrictions ...")

    for bitness, case_id, _ in GOLDEN_VALUES:
        free_bits = bitness - 1
        sample_size = 2 * free_bits + 1

        value = library.bb_case_restrictions(
            bitness,
            case_id,
            0,
        ).decode("ascii")
        assert len(value) == bitness * 2 * sample_size

        for fixed_bit_id in range(bitness):
            for fixed_bit_value in range(2):
                restriction_id = fixed_bit_id * 2 + fixed_bit_value
                offset = restriction_id * sample_size
                value_chunk = value[offset : offset + sample_size]

                full_input = list("0" * bitness)
                full_input[fixed_bit_id] = str(fixed_bit_value)
                for coord, bit_value in enumerate(value_chunk[:free_bits]):
                    full_bit_id = coord if coord < fixed_bit_id else coord + 1
                    full_input[full_bit_id] = bit_value
                full_input = "".join(full_input)

                direct_value = case_value(library, bitness, case_id, full_input)
                expected = (
                    value_chunk[:free_bits]
                    + direct_value[bitness]
                    + "".join(
                        direct_value[bitness + 1 + full_bit_id]
                        for full_bit_id in range(bitness)
                        if full_bit_id != fixed_bit_id
                    )
                )

                assert value_chunk == expected


def parity_value(library, bitness, input_bits):
    return library.bb_parity_value(
        bitness,
        input_bits.encode("ascii"),
    ).decode("ascii")


def test_parity_value(library):
    print(f"Check bb_parity_value ...")

    for input_bits in ["0", "1", "0101", "1110", "101101"]:
        bitness = len(input_bits)
        parity = str(input_bits.count("1") % 2)
        expected = input_bits + parity + "".join(
            str(1 - int(parity))
            for _ in input_bits
        )
        assert parity_value(library, bitness, input_bits) == expected


def test_parity_restrictions(library):
    print(f"Check bb_parity_restrictions ...")

    for bitness in [1, 2, 4, 6]:
        free_bits = bitness - 1
        sample_size = 2 * free_bits + 1

        value = library.bb_parity_restrictions(
            bitness,
            0,
        ).decode("ascii")
        assert len(value) == bitness * 2 * sample_size

        for fixed_bit_id in range(bitness):
            for fixed_bit_value in range(2):
                restriction_id = fixed_bit_id * 2 + fixed_bit_value
                offset = restriction_id * sample_size
                value_chunk = value[offset : offset + sample_size]

                full_input = list("0" * bitness)
                full_input[fixed_bit_id] = str(fixed_bit_value)
                for coord, bit_value in enumerate(value_chunk[:free_bits]):
                    full_bit_id = coord if coord < fixed_bit_id else coord + 1
                    full_input[full_bit_id] = bit_value
                full_input = "".join(full_input)

                direct_value = parity_value(library, bitness, full_input)
                expected = (
                    value_chunk[:free_bits]
                    + direct_value[bitness]
                    + "".join(
                        direct_value[bitness + 1 + full_bit_id]
                        for full_bit_id in range(bitness)
                        if full_bit_id != fixed_bit_id
                    )
                )

                assert value_chunk == expected


def test_case_depth(library):
    print(f"Check bb_case_depth ...")

    for bitness, case_id, _ in GOLDEN_VALUES:
        depth = library.bb_case_depth(bitness, case_id)
        assert 0 <= depth <= bitness


def test_circuit_discovery(library):
    print(f"Check bb_circuit discovery ...")

    sets = split_list(library.bb_circuit_sets())
    assert "iscas85" in sets
    assert "iscas87" in sets

    assert split_list(library.bb_circuit_cases(b"iscas85")) == ["c17"]
    assert split_list(library.bb_circuit_cases(b"iscas87")) == ["s27"]


def test_circuit_metadata(library):
    print(f"Check bb_circuit metadata ...")

    assert library.bb_circuit_inputs(b"iscas85", b"c17") == 5
    assert library.bb_circuit_outputs(b"iscas85", b"c17") == 2

    assert library.bb_circuit_inputs(b"iscas87", b"s27") == 7
    assert library.bb_circuit_outputs(b"iscas87", b"s27") == 1


def test_circuit_value(library):
    print(f"Check bb_circuit_value ...")

    c17 = circuit_value(library, "iscas85", "c17", "00000")
    assert len(c17) == 5 + 2 * 6
    assert c17 == circuit_value(library, "iscas85", "c17", "00000")

    s27 = circuit_value(library, "iscas87", "s27", "0000000")
    assert len(s27) == 7 + 1 * 8
    assert s27 == circuit_value(library, "iscas87", "s27", "0000000")


if __name__ == "__main__":
    library = load_library()
    test_case_value(library)
    test_case_restrictions(library)
    test_parity_value(library)
    test_parity_restrictions(library)
    test_case_depth(library)
    test_circuit_discovery(library)
    test_circuit_metadata(library)
    test_circuit_value(library)
