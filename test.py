import ctypes
from pathlib import Path


LIBRARY = Path(__file__).resolve().parent / "build" / "libbb.so"
CIRCUITS = Path(__file__).resolve().parent / "circuits"

TABLE_SOLVABLE_CASES = [
    (4, 0, "0101", "010100000", 0, 0),
    (4, 3190, "0001", "000100100", 4, 7),
    (4, 11304, "1101", "110111001", 4, 6),
    (5, 3261348405, "11000", "11000010010", 5, 14),
    (5, 390455940, "01001", "01001101111", 5, 16),
    (6, 2547012052, "110111", "1101110000000", 6, 16),
    (6, 883941716, "011111", "0111110000000", 6, 17),
    (7, 42, "0101010", "010101000010010", 7, 59),
    (7, 239, "1010101", "101010110101101", 7, 58),
    (
        11,
        23901,
        "01010101010",
        "01010101010011100010111",
        11,
        842,
    ),
    (
        16,
        42,
        "0101010101010101",
        "010101010101010100111110101000000",
        16,
        26467,
    ),
    (
        16,
        239566,
        "1010101010101010",
        "101010101010101000110101001010010",
        16,
        26543,
    ),
]

TABLE_BIG_CASES = [
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

TREE_CASES = [
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

def load_library():
    library = ctypes.CDLL(str(LIBRARY))

    library.bb_tree_cases_number.argtypes = [ctypes.c_uint16]
    library.bb_tree_cases_number.restype = ctypes.c_size_t

    library.bb_table_solvable_bitness.argtypes = []
    library.bb_table_solvable_bitness.restype = ctypes.c_uint16

    library.bb_tree_nodes.argtypes = [ctypes.c_uint16, ctypes.c_size_t]
    library.bb_tree_nodes.restype = ctypes.c_size_t

    library.bb_tree_depth.argtypes = [ctypes.c_uint16, ctypes.c_size_t]
    library.bb_tree_depth.restype = ctypes.c_size_t

    library.bb_tree_value.argtypes = [
        ctypes.c_uint16,
        ctypes.c_size_t,
        ctypes.c_char_p,
    ]
    library.bb_tree_value.restype = ctypes.c_char_p

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


def tree_value(library, bitness, case_id, input_bits):
    return library.bb_tree_value(
        bitness,
        case_id,
        input_bits.encode("ascii"),
    ).decode("ascii")


def table_value(library, bitness, case_id, input_bits):
    return library.bb_table_value(
        bitness,
        case_id,
        input_bits.encode("ascii"),
    ).decode("ascii")


def assert_case_restrictions_consistent(
    library,
    restrictions_func,
    value_func,
    restrictions_name,
    bitness,
    case_id,
):
    free_bits = bitness - 1
    sample_size = 2 * free_bits + 1
    reps = 3
    packed_input = "".join(
        "1" if (restriction_id + rep + coord) % 2 else "0"
        for restriction_id in range(bitness * 2)
        for rep in range(reps)
        for coord in range(free_bits)
    )

    value = restrictions_func(
        bitness,
        case_id,
        packed_input.encode("ascii"),
    ).decode("ascii")
    assert len(value) == bitness * 2 * reps * sample_size

    for fixed_bit_id in range(bitness):
        for fixed_bit_value in range(2):
            restriction_id = fixed_bit_id * 2 + fixed_bit_value

            for rep in range(reps):
                offset = (restriction_id * reps + rep) * sample_size
                value_chunk = value[offset : offset + sample_size]
                input_offset = (restriction_id * reps + rep) * free_bits
                free_input = packed_input[input_offset : input_offset + free_bits]
                assert value_chunk[:free_bits] == free_input, (
                    f"{restrictions_name}({bitness}, {case_id}) input mismatch: "
                    f"fixed_bit_id={fixed_bit_id}, "
                    f"fixed_bit_value={fixed_bit_value}, "
                    f"rep={rep}, "
                    f"actual={value_chunk[:free_bits]}, expected={free_input}"
                )

                full_input = list("0" * bitness)
                full_input[fixed_bit_id] = str(fixed_bit_value)
                for coord, bit_value in enumerate(free_input):
                    full_bit_id = coord if coord < fixed_bit_id else coord + 1
                    full_input[full_bit_id] = bit_value
                full_input = "".join(full_input)

                direct_value = value_func(library, bitness, case_id, full_input)
                expected = (
                    value_chunk[:free_bits]
                    + direct_value[bitness]
                    + "".join(
                        direct_value[bitness + 1 + full_bit_id]
                        for full_bit_id in range(bitness)
                        if full_bit_id != fixed_bit_id
                    )
                )

                assert value_chunk == expected, (
                    f"{restrictions_name}({bitness}, {case_id}) mismatch: "
                    f"fixed_bit_id={fixed_bit_id}, "
                    f"fixed_bit_value={fixed_bit_value}, "
                    f"rep={rep}, "
                    f"actual={value_chunk}, expected={expected}"
                )


def assert_case_consistent(library, value_func, value_name, bitness, case_id, input_bits):
    value = value_func(library, bitness, case_id, input_bits)
    assert len(value) == 2 * bitness + 1, (
        f"{value_name}({bitness}, {case_id}, {input_bits}) length: "
        f"actual={len(value)}, expected={2 * bitness + 1}, value={value}"
    )
    assert value[:bitness] == input_bits, (
        f"{value_name}({bitness}, {case_id}, {input_bits}) input prefix: "
        f"actual={value[:bitness]}, expected={input_bits}, value={value}"
    )

    repeat_value = value_func(library, bitness, case_id, input_bits)
    assert value == repeat_value, (
        f"{value_name}({bitness}, {case_id}, {input_bits}) is not stable: "
        f"first={value}, second={repeat_value}"
    )

    for bit_id in range(bitness):
        flipped = list(input_bits)
        flipped[bit_id] = "1" if flipped[bit_id] == "0" else "0"
        flipped_value = value_func(library, bitness, case_id, "".join(flipped))
        assert value[bitness + 1 + bit_id] == flipped_value[bitness], (
            f"{value_name}({bitness}, {case_id}, {input_bits}) "
            f"flip bit {bit_id}: sampled={value[bitness + 1 + bit_id]}, "
            f"direct={flipped_value[bitness]}, flipped_input={''.join(flipped)}"
        )

    return value


def test_tree_cases(library):
    print(f"Check tree cases ...")

    for case in TREE_CASES:
        (
            bitness,
            case_id,
            input_bits,
            expected_value,
            expected_depth,
            expected_nodes,
        ) = case
        value = assert_case_consistent(
            library,
            tree_value,
            "bb_tree_value",
            bitness,
            case_id,
            input_bits,
        )
        assert value == expected_value, (
            f"bb_tree_value({bitness}, {case_id}, {input_bits}): "
            f"actual={value}, expected={expected_value}"
        )

        depth = library.bb_tree_depth(bitness, case_id)
        assert depth == expected_depth, (
            f"bb_tree_depth({bitness}, {case_id}): "
            f"actual={depth}, expected={expected_depth}"
        )
        nodes = library.bb_tree_nodes(bitness, case_id)
        assert nodes == expected_nodes, (
            f"bb_tree_nodes({bitness}, {case_id}): "
            f"actual={nodes}, expected={expected_nodes}"
        )

        assert_case_restrictions_consistent(
            library,
            library.bb_tree_restrictions,
            tree_value,
            "bb_tree_restrictions",
            bitness,
            case_id,
        )

        flipped = list(input_bits)
        flipped[0] = "1" if flipped[0] == "0" else "0"
        flipped = "".join(flipped)

        first = tree_value(library, bitness, case_id, input_bits)
        other = tree_value(library, bitness, case_id, flipped)
        first_repeat = tree_value(library, bitness, case_id, input_bits)
        other_repeat = tree_value(library, bitness, case_id, flipped)
        assert first == first_repeat, (
            f"bb_tree_value({bitness}, {case_id}, {input_bits}) repeat: "
            f"first={first}, second={first_repeat}"
        )
        assert other == other_repeat, (
            f"bb_tree_value({bitness}, {case_id}, {flipped}) repeat: "
            f"first={other}, second={other_repeat}"
        )


def test_table_solvable_cases(library):
    print(f"Check solvable table cases ...")

    for (
        bitness,
        case_id,
        input_bits,
        expected_value,
        expected_depth,
        expected_nodes,
    ) in TABLE_SOLVABLE_CASES:
        value = assert_case_consistent(
            library,
            table_value,
            "bb_table_value",
            bitness,
            case_id,
            input_bits,
        )
        assert value == expected_value, (
            f"bb_table_value({bitness}, {case_id}, {input_bits}): "
            f"actual={value}, expected={expected_value}"
        )

        depth = library.bb_table_depth(bitness, case_id)
        assert depth == expected_depth, (
            f"bb_table_depth({bitness}, {case_id}): "
            f"actual={depth}, expected={expected_depth}"
        )
        nodes = library.bb_table_nodes(bitness, case_id)
        assert nodes == expected_nodes, (
            f"bb_table_nodes({bitness}, {case_id}): "
            f"actual={nodes}, expected={expected_nodes}"
        )
        if (bitness, case_id) == TABLE_BIG_CASES[0][:2]:
            assert_case_restrictions_consistent(
                library,
                library.bb_table_restrictions,
                table_value,
                "bb_table_restrictions",
                bitness,
                case_id,
            )


def test_table_big_cases(library):
    print(f"Check big table cases ...")

    for bitness, case_id, input_bits, expected_value in TABLE_BIG_CASES:
        assert bitness > library.bb_table_solvable_bitness(), bitness
        value = assert_case_consistent(
            library,
            table_value,
            "bb_table_value",
            bitness,
            case_id,
            input_bits,
        )
        assert value == expected_value, (
            f"bb_table_value({bitness}, {case_id}, {input_bits}): "
            f"actual={value}, expected={expected_value}"
        )
        assert_case_restrictions_consistent(
            library,
            library.bb_table_restrictions,
            table_value,
            "bb_table_restrictions",
            bitness,
            case_id,
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
    test_tree_cases(library)
    test_table_solvable_cases(library)
    test_table_big_cases(library)
    test_circuit_discovery(library)
    test_circuit_metadata(library)
    test_circuit_value(library)
