import ctypes
from pathlib import Path


LIBRARY = Path(__file__).resolve().parent / "build" / "libbb.so"
CIRCUITS = Path(__file__).resolve().parent / "circuits"

SOLVABLE_CASES = [
    (4, 0, "0101", "010100000", 0, 0),
    (4, 3190, "0001", "000100100", 4, 7),
    (4, 11304, "1101", "110111001", 4, 6),
    (5, 3261348405, "11000", "11000010010", 5, 14),
    (5, 390455940, "01001", "01001101111", 5, 16),
    (6, 2547012052, "110111", "1101110000000", 6, 16),
    (6, 883941716, "011111", "0111110000000", 6, 17),
    (7, 42, "0101010", "010101000010010", 7, 77),
    (
        16,
        42,
        "0101010101010101",
        "010101010101010100111110101000000",
        16,
        40170,
    ),
    (
        17,
        42,
        "01010101010101010",
        "01010101010101010000010000100001010",
        10,
        42,
    ),
    (
        24,
        188,
        "110010100111000101010011",
        "1100101001110001010100111110110110111111111101111",
        19,
        188,
    ),
    (
        32,
        320,
        "01010101010101010101010101010101",
        "01010101010101010101010101010101010000101000000000000100000001010",
        16,
        320,
    ),
    (
        48,
        480,
        "110010101100101011001010110010101100101011001010",
        "1100101011001010110010101100101011001010110010100000001001000000000000000000000001000000000000100",
        23,
        480,
    ),
    (
        64,
        640,
        "0011010100110101001101010011010100110101001101010011010100110101",
        "001101010011010100110101001101010011010100110101001101010011010111111111010111111111111111111111111111111101111111111111111111111",
        22,
        640,
    ),
    (
        100,
        1000,
        "0100110011010011001101001100110100110011010011001101001100110100110011010011001101001100110100110011",
        "010011001101001100110100110011010011001101001100110100110011010011001101001100110100110011010011001111111110111111111111111111111111110111111110111111111111111111111111111111111111111111111111111111111",
        23,
        1000,
    ),
    (
        128,
        1280,
        "01101001100101100110100110010110011010011001011001101001100101100110100110010110011010011001011001101001100101100110100110010110",
        "01101001100101100110100110010110011010011001011001101001100101100110100110010110011010011001011001101001100101100110100110010110111111111111111101111111111111111111011111111111111111111111111111111111111111111111111111111111111111111111110111111111111111111",
        23,
        1280,
    ),
]

RANDOM_CASES = [
    (
        17,
        42,
        "01010101010101010",
        "01010101010101010111000000111000110",
    ),
    (
        24,
        188,
        "110010100111000101010011",
        "1100101001110001010100111010111010101001011101101",
    ),
    (
        32,
        320,
        "01010101010101010101010101010101",
        "01010101010101010101010101010101011000000101101101000111000101001",
    ),
    (
        48,
        480,
        "110010101100101011001010110010101100101011001010",
        "1100101011001010110010101100101011001010110010100001011011000001001010110100000110011111101011011",
    ),
    (
        64,
        640,
        "0011010100110101001101010011010100110101001101010011010100110101",
        "001101010011010100110101001101010011010100110101001101010011010101110100101111001010111011010001111101000011000011001000011110110",
    ),
    (
        100,
        1000,
        "0100110011010011001101001100110100110011010011001101001100110100110011010011001101001100110100110011",
        "010011001101001100110100110011010011001101001100110100110011010011001101001100110100110011010011001101101100011010001101100111110110110000101011001010010010000100100011010100100110010101010011110011001",
    ),
    (
        128,
        1280,
        "01101001100101100110100110010110011010011001011001101001100101100110100110010110011010011001011001101001100101100110100110010110",
        "01101001100101100110100110010110011010011001011001101001100101100110100110010110011010011001011001101001100101100110100110010110001100110000111111001001101111110101010111011001001011111100100001011001001101001001011011011110111000110101111011101010101101100",
    ),
]

def load_library():
    library = ctypes.CDLL(str(LIBRARY))

    library.bb_tree_cases_number.argtypes = [ctypes.c_uint16]
    library.bb_tree_cases_number.restype = ctypes.c_size_t

    library.bb_table_solvable_bitness.argtypes = []
    library.bb_table_solvable_bitness.restype = ctypes.c_uint16

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

    library.bb_table_value.argtypes = [
        ctypes.c_uint16,
        ctypes.c_size_t,
        ctypes.c_char_p,
    ]
    library.bb_table_value.restype = ctypes.c_char_p

    library.bb_table_nodes.argtypes = [ctypes.c_uint16, ctypes.c_size_t]
    library.bb_table_nodes.restype = ctypes.c_size_t

    library.bb_table_depth.argtypes = [ctypes.c_uint16, ctypes.c_size_t]
    library.bb_table_depth.restype = ctypes.c_size_t

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


def case_value_rnd(library, bitness, case_id, input_bits):
    return library.bb_table_value(
        bitness,
        case_id,
        input_bits.encode("ascii"),
    ).decode("ascii")


def test_case_restrictions(library):
    print(f"Check bb_case_restrictions ...")

    for bitness, case_id, _, _, _, _ in SOLVABLE_CASES:
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


def assert_case_consistent(
    library,
    bitness,
    case_id,
    input_bits,
    expected_value,
    expected_depth,
    expected_nodes,
):
    value = case_value(library, bitness, case_id, input_bits)
    assert len(value) == 2 * bitness + 1
    assert value[:bitness] == input_bits
    assert value == expected_value
    assert value == case_value(library, bitness, case_id, input_bits)
    assert library.bb_gen_depth(bitness, case_id) == expected_depth
    assert library.bb_gen_nodes(bitness, case_id) == expected_nodes

    for bit_id in range(bitness):
        flipped = list(input_bits)
        flipped[bit_id] = "1" if flipped[bit_id] == "0" else "0"
        flipped_value = case_value(library, bitness, case_id, "".join(flipped))
        assert value[bitness + 1 + bit_id] == flipped_value[bitness]


def test_solvable_cases(library):
    print(f"Check solvable generated cases ...")

    for case in SOLVABLE_CASES:
        bitness, case_id, input_bits, _, _, _ = case
        assert_case_consistent(library, *case)

        flipped = list(input_bits)
        flipped[0] = "1" if flipped[0] == "0" else "0"
        flipped = "".join(flipped)

        first = case_value(library, bitness, case_id, input_bits)
        other = case_value(library, bitness, case_id, flipped)
        assert first == case_value(library, bitness, case_id, input_bits)
        assert other == case_value(library, bitness, case_id, flipped)
        assert first == case_value(library, bitness, case_id, input_bits)


def test_random_cases(library):
    print(f"Check bb_table_value ...")

    for bitness, case_id, input_bits, expected_value in RANDOM_CASES:
        assert bitness > library.bb_table_solvable_bitness(), bitness
        value = case_value_rnd(library, bitness, case_id, input_bits)
        assert len(value) == 2 * bitness + 1
        assert value[:bitness] == input_bits
        assert value == expected_value
        assert value == case_value_rnd(library, bitness, case_id, input_bits)

        for bit_id in range(bitness):
            flipped = list(input_bits)
            flipped[bit_id] = "1" if flipped[bit_id] == "0" else "0"
            flipped_value = case_value_rnd(library, bitness, case_id, "".join(flipped))
            assert value[bitness + 1 + bit_id] == flipped_value[bitness]


def test_table_metrics(library):
    print(f"Check bb_table_nodes/depth ...")

    table_metrics = [
        (library.bb_table_nodes, 4, 3190, 7),
        (library.bb_table_depth, 4, 3190, 4),
        (library.bb_table_nodes, 7, 42, 59),
        (library.bb_table_depth, 7, 42, 7),
        (library.bb_table_nodes, 7, 239, 58),
        (library.bb_table_depth, 7, 239, 7),
        (library.bb_table_nodes, 11, 23901, 842),
        (library.bb_table_depth, 11, 23901, 11),
        (library.bb_table_nodes, 16, 239566, 26543),
        (library.bb_table_depth, 16, 239566, 16),
    ]

    for function, bitness, case_id, expected in table_metrics:
        actual = function(bitness, case_id)
        assert actual == expected, (
            f"{function.__name__}({bitness}, {case_id}): "
            f"actual={actual}, expected={expected}"
        )


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
    test_case_restrictions(library)
    test_solvable_cases(library)
    test_random_cases(library)
    test_table_metrics(library)
    test_circuit_discovery(library)
    test_circuit_metadata(library)
    test_circuit_value(library)
