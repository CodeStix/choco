export extern func printDouble(d)
export extern func print(c)

export func getValue() {
    return 100000
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

    let str = "nice!"

    let value = getValue()
    printFibbonaci(value)
    return value
}
