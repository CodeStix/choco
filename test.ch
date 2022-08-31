
export extern func printDouble(float: Float32): Float32

func getValue(): Float32 {
    return 100
}

export func printFibbonaci(limit: Float64) {
    if limit == 69 | limit == 420 {
        return
    }

    let a = 0
    let b = 1
    let n = 0
    while n < limit {
        n = a + b
        a = b
        b = n
        printDouble(-n)
    }
}

func printBits(integer: Int64) {
    let i: Int32 = 31
    while i >= 0 {
        printDouble((integer & (1 << i)) != 0)
        i = i -1
    }
}

func getInteger(): Int64 {
    return 0b11111111
}

export func main() {
    printFibbonaci(420)
    printFibbonaci(69) 

    // Nice
    printBits(127)
    printDouble(getInteger())
}