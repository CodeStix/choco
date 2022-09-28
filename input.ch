
export extern func printDouble(float: Float32): Float32

func getInteger(): Int64 {
    return 16
}

struct NestedStruct value {
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

func modifyStruct(str: NestedStruct) {
    str.bigNumber = str.bigNumber * 2
}

export func main() {
    let b = TestStruct {
        original: Int32 getInteger()
        val: Int32 getInteger()
        data: value {
            bigNumber: Float64 1234,
        }
    } 

    if ((b.val == 100) || (b.val == 200)) {
        return
    }

    while (b.val > 0) {
        b.val = Int32 (b.val - 1)
        printStruct(b)
    }

    printDouble(recurse(Int32 60))
    printDouble(b.original)

    modifyStruct(b.data)
    printDouble(b.data.bigNumber)

    let structCopy = b.data
    structCopy.bigNumber = Float64 100
    printDouble(structCopy.bigNumber)
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