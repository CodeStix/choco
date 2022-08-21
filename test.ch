export extern func printDouble(d)

export func getValue() {
    return 10000000000000000
}

export func printFibbonaci(limit) {
    let a = 0
    let b = 1
    let n = 0
    while n < limit {
        n = a + b
        a = b
        b = n
        printDouble(n)
    }
    return 0
}

export func main() {

    let value = getValue()
    printFibbonaci(value)
    return value
}
