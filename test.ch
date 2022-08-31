
export extern func printDouble(float: Float32): Float32

func getValue(): Float32 {
    return 100
}

export func printFibbonaci(limit: Float32) {
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
    let a = getValue()
    while(a > 1) {
        a = a - 1
        printDouble(a)
        if a < 50 {
            return
        }
    }

    printFibbonaci(10000)
}