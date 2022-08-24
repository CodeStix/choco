
export extern func printDouble(float: float32): float32

func test(v: float32): float32 {
    return v + 1
}

export func main() {
    let value = test(100)
    printDouble(value)
    printDouble(value * 2)
    printDouble(value * 4)
    return
}