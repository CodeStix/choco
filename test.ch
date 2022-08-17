

func printDouble(d) extern

func pow(a) {
    return a * a * a
}

func getA() {
    return 400
}

func getB() {
    return 300
}

func testmain() {
    let a = getA()
    let b = getB()
    let r = 0
    if a > b {
        r = 1
        printDouble(123)
    }
    printDouble(r)
    return r
}
