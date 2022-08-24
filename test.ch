
export extern func printDouble(float: float32): float32

func getValue(): float32 {
    return 100
}

func test(v: float32): float32 {
    return v + 1
}

export func main() {

    let integer: int32 = getValue()
    let float: float32 = integer
    printDouble(float)

    let value: float32 = test(100) + 200
    printDouble(value)
    printDouble(value * 2)
    printDouble(value * 4)
    return
}