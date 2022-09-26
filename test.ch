
export extern func printDouble(float: Float32): Float32

func getInteger(): Int64 {
    return 16
}

struct NestedStruct {
    bigNumber: Float64
}

struct TestStruct {
    original: Int32
    val: Int32
    data: NestedStruct
}

func printStruct(str: TestStruct) {
    printDouble(str.val)
}

export func main() {
    let b = TestStruct {
        original: Int32 getInteger()
        val: Int32 getInteger()
        data: {
            bigNumber: Float64 1234,
        }
    } 

    if ((b.val == 100) || (b.val == 200)) {
        return
    }

    while (b.val > 0) {
        b.val = b.val - 1
        printStruct(b)
    }

    printDouble(recurse(50))
    printDouble(b.original)
    printDouble(b.data.bigNumber)
}

func recurse(a: Int32): Int32 {
    if (a == 0) {
        return 0
    }
    else {
        return a + recurse(a - 1)
    }
}