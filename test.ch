
export extern func printDouble(float: Float32): Float32

func getValue(): Float32 {
    return 100
}

export func main() {
    let a = getValue()
    while(a > 1) {
        a = a - 1
        printDouble(a)
        if a < 50 {
            return
        }
    }
    return
}