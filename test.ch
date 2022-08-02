
func printDouble(d) extern
func sin(a) extern

func pow(a) {
    return a * a
}

func testmain() {
    let c = pow(25) + pow(50) 
    printDouble(c + 1)
    return c
}
