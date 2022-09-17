
export extern func printDouble(float: Float32): Float32

func getInteger(): Int64 {
    return 5001
}

export func main() {
    let b = {
        value: getInteger(),
        moreValue: getInteger()
    }
    
    printDouble(b.value + b.moreValue)
}