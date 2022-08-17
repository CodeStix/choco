

func printDouble(d) extern

func getValue() {
    return 100
}

func testmain() {

    let r = getValue()
    if r > 50 {
        printDouble(1)
        while(r > 0) {
            printDouble(r)
            r = r - 1
        }
    }
    else {
        printDouble(0)
    }
    
    return r
}

