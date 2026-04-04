from ble import prepare_write_bytes, Ble_Cmd


def b_str(cmd: Ble_Cmd):
    return str(cmd.value)


def test_writes_load_command():
    args = f"{b_str(Ble_Cmd.LOAD_ROW_CMD)} 1 4"
    res = prepare_write_bytes(args, Ble_Cmd.LOAD_ROW_CMD)
    assert res == bytes([Ble_Cmd.LOAD_ROW_CMD.value, 1, 4])


def test_writes_write_cell_command():
    check_val = int(5000).to_bytes(2, "little")
    args = f"{b_str(Ble_Cmd.WRITE_CELL_CMD)} 4 2 3 5000"

    res = prepare_write_bytes(args, Ble_Cmd.WRITE_CELL_CMD)
    assert res == bytes(
        [Ble_Cmd.WRITE_CELL_CMD.value, 4, 2, 3, check_val[0], check_val[1]]
    )


def test_writes_write_row_command():
    vals = [1000, 2000, 3000, 4000, 5000]
    byteArr = b"".join([int(x).to_bytes(2, "little") for x in vals])

    args = f"{b_str(Ble_Cmd.WRITE_ROW_CMD)} 11 2 1000 2000 3000 4000 5000"
    res = prepare_write_bytes(args, Ble_Cmd.WRITE_ROW_CMD)
    correct = True
    start_len = 3
    for i in range(6):
        if res[start_len + i] != byteArr[i]:
            correct = False
    assert correct
