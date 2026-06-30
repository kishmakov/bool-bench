import ctypes
from pathlib import Path


LIBRARY = Path(__file__).resolve().parent / "build" / "libbb.so"
CIRCUITS = Path(__file__).resolve().parent / "circuits"

REFERENCE_CASES = [
    (4,    0, "0101", "010100000"),
    (4, 3190, "0001", "000100100"),
    (4, 11304, "1101", "110111001"),
    (5, 3261348405, "11000", "11000010010"),
    (5, 390455940, "01001", "01001101111"),
    (6, 2547012052, "110111", "1101110000000"),
    (6, 883941716, "011111", "0111110000000"),
    (7, 42, "0101010", "010101000010010"),
    (16, 42, "0101010101010101", "010101010101010100111110101000000"),
]

SPARSE_CASES = [
    (17, 42, "01010101010101010"),
    (24, 188, "110010100111000101010011"),
]

def load_library():
    library = ctypes.CDLL(str(LIBRARY))

    library.bb_gen_cases_number.argtypes = [ctypes.c_uint16]
    library.bb_gen_cases_number.restype = ctypes.c_size_t

    library.bb_gen_nodes.argtypes = [ctypes.c_uint16, ctypes.c_size_t]
    library.bb_gen_nodes.restype = ctypes.c_size_t

    library.bb_gen_depth.argtypes = [ctypes.c_uint16, ctypes.c_size_t]
    library.bb_gen_depth.restype = ctypes.c_size_t

    library.bb_gen_value.argtypes = [
        ctypes.c_uint16,
        ctypes.c_size_t,
        ctypes.c_char_p,
    ]
    library.bb_gen_value.restype = ctypes.c_char_p

    library.bb_case_restrictions.argtypes = [
        ctypes.c_uint16,
        ctypes.c_size_t,
        ctypes.c_size_t,
    ]
    library.bb_case_restrictions.restype = ctypes.c_char_p

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


def expected_circuit_sets():
    return sorted(
        path.name
        for path in CIRCUITS.iterdir()
        if path.is_dir() and any(path.glob("*.aig"))
    )


def expected_circuit_cases(set_name):
    return sorted(path.stem for path in (CIRCUITS / set_name).glob("*.aig"))


def read_aig_metadata(set_name, case_name):
    path = CIRCUITS / set_name / f"{case_name}.aig"
    with path.open("rb") as file:
        header = file.readline().decode("ascii").split()

    assert header[0] == "aig"
    assert len(header) >= 6

    inputs = int(header[2])
    latches = int(header[3])
    outputs = int(header[4])
    bads = int(header[6]) if len(header) > 6 else 0
    return inputs + latches, outputs + bads


def circuit_value(library, set_name, case_name, input_state):
    return library.bb_circuit_value(
        set_name.encode("ascii"),
        case_name.encode("ascii"),
        input_state.encode("ascii"),
    ).decode("ascii")


def case_value(library, bitness, case_id, input_bits):
    return library.bb_gen_value(
        bitness,
        case_id,
        input_bits.encode("ascii"),
    ).decode("ascii")


def test_case_value(library):
    print(f"Check bb_gen_value ...")

    for bitness, case_id, input_bits, expected in REFERENCE_CASES:
        assert case_id < library.bb_gen_cases_number(bitness)
        assert case_value(library, bitness, case_id, input_bits) == expected


def test_case_restrictions(library):
    print(f"Check bb_case_restrictions ...")

    cases = [(bitness, case_id) for bitness, case_id, _, _ in REFERENCE_CASES]
    cases += [(bitness, case_id) for bitness, case_id, _ in SPARSE_CASES]
    for bitness, case_id in cases:
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


def test_case_depth(library):
    print(f"Check bb_gen_depth and bb_gen_nodes ...")

    for bitness, case_id, _, _ in REFERENCE_CASES:
        depth = library.bb_gen_depth(bitness, case_id)
        nodes = library.bb_gen_nodes(bitness, case_id)
        assert 0 <= depth <= bitness
        assert nodes >= 0


def assert_case_consistent(library, bitness, case_id, input_bits, check_metrics):
    value = case_value(library, bitness, case_id, input_bits)
    assert len(value) == 2 * bitness + 1
    assert value[:bitness] == input_bits
    assert value == case_value(library, bitness, case_id, input_bits)
    if check_metrics:
        assert 0 <= library.bb_gen_depth(bitness, case_id) <= bitness
        assert library.bb_gen_nodes(bitness, case_id) >= 0

    for bit_id in range(bitness):
        flipped = list(input_bits)
        flipped[bit_id] = "1" if flipped[bit_id] == "0" else "0"
        flipped_value = case_value(library, bitness, case_id, "".join(flipped))
        assert value[bitness + 1 + bit_id] == flipped_value[bitness]


def test_reference_cases(library):
    print(f"Check reference cases ...")

    for bitness, case_id, input_bits, _ in REFERENCE_CASES:
        assert_case_consistent(library, bitness, case_id, input_bits, True)


def test_sparse_cases(library):
    print(f"Check sparse generated cases ...")

    for bitness, case_id, input_bits in SPARSE_CASES:
        assert_case_consistent(library, bitness, case_id, input_bits, False)

        flipped = list(input_bits)
        flipped[0] = "1" if flipped[0] == "0" else "0"
        flipped = "".join(flipped)

        first = case_value(library, bitness, case_id, input_bits)
        other = case_value(library, bitness, case_id, flipped)
        assert first == case_value(library, bitness, case_id, input_bits)
        assert other == case_value(library, bitness, case_id, flipped)
        assert first == case_value(library, bitness, case_id, input_bits)


def test_circuit_discovery(library):
    print(f"Check bb_circuit discovery ...")

    sets = split_list(library.bb_circuit_sets())
    assert sets == expected_circuit_sets()

    for set_name in sets:
        assert split_list(library.bb_circuit_cases(set_name.encode("ascii"))) == (
            expected_circuit_cases(set_name)
        )


def test_circuit_metadata(library):
    print(f"Check bb_circuit metadata ...")

    for set_name in expected_circuit_sets():
        for case_name in expected_circuit_cases(set_name):
            expected_inputs, expected_outputs = read_aig_metadata(set_name, case_name)
            assert library.bb_circuit_inputs(
                set_name.encode("ascii"),
                case_name.encode("ascii"),
            ) == expected_inputs
            assert library.bb_circuit_outputs(
                set_name.encode("ascii"),
                case_name.encode("ascii"),
            ) == expected_outputs

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
    test_case_depth(library)
    test_reference_cases(library)
    test_sparse_cases(library)
    test_circuit_discovery(library)
    test_circuit_metadata(library)
    test_circuit_value(library)
