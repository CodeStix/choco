
export extern func printDouble(float: Float32): Float32

func getValue(): Float32 {
    return 100
}

func test(v: Float32): Float32 {
    return v + 1
}

export func main() {

    let integer: Int64 = getValue()
    let float: Float32 = integer
    printDouble(float)

    let value: Float32 = test(100) + 200
    printDouble(value)
    printDouble(value * 2)
    printDouble(value * 4)
    return
}