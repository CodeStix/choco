
export extern func printDouble(float: Float32): Float32

func getInteger(): Int64 {
    return 5001
}

export func main() {
    let b = {
        val: getInteger()
    } 

    if ((b.val == 100) || (b.val == 200)) {
        return
    }

    while (b.val > 0) {
        b.val = b.val + 1
        printDouble(b.val)
    }
}
