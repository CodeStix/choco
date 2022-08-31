
export extern func printDouble(float: Float32): Float32

func getValue(): Float32 {
    return 100
}

export func printFibbonaci(limit: Float64) {
    let a = 0
    let b = 1
    let n = 0
    while n < limit {
        n = a + b
        a = b
        b = n
        printDouble(n)
    }
}

export func main() {

    printFibbonaci(1000000)
}