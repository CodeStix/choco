

export extern func printDouble(d)

func getValue() {
    return 20
}

export func testmain() {

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
