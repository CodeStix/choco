
export extern func printDouble(float: Float32): Float32

func getInteger(): Int64 {
    return 1234:Int64
}

export func main() {

    let a = {
        value: 100
    }

    printDouble(getInteger())
    return
}