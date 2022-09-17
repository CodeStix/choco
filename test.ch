
export extern func printDouble(float: Float32): Float32

func getInteger(): Int64 {
    return 5001
}

export func main() {
    let b = {
        value: 100
    }

    if ((b.value == 100) || (b.value == 200)) {
        return
    }

    while b.value > 0 {
        b.value = b.value + -1
        printDouble(b.value)
    }
}