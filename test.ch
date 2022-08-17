

func printDouble(d) extern

func getValue() {
    return 100000
}

func testmain() {

    let value = getValue()

    while(value > 0) {
        printDouble(value)
        value = value - 1
    }
    else {
        printDouble(0)
    }

    return value
}
