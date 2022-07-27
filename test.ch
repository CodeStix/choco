
func print(str) extern

func testFunction(a) {
    let r = a * 123
    r = r * 5
    return r + 5
}

func main() {
    let b = testFunction(50)
    return b - 5
}