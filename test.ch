
export extern func printDouble(float: Float32): Float32

func getInteger(): Int64 {
    return 16
}

struct NestedStruct value {
    bigNumber: Float32
}

struct TestStruct {
    original: Int32
    val: Int32
    data: NestedStruct
}


struct ReturnedStruct {
    x: Int32
    y: Int32
}

func returnStruct(): ReturnedStruct {
    return {
        x: Int32 123
        y: Int32 456
    }
}

func printStruct(str: TestStruct) {
    printDouble(Float32 str.val)
}

func modifyStruct(str: NestedStruct) {
    str.bigNumber = str.bigNumber * 2
    printDouble(str.bigNumber)
}

export func main() {
    let b = TestStruct {
        original: Int32 getInteger()
        val: Int32 getInteger()
        data: value {
            bigNumber: Float32 1234,
        }
    } 

    if ((b.val == 100) || (b.val == 200)) {
        return
    }

    while (b.val > 0) {
        b.val = Int32 (b.val - 1)
        printStruct(b)
    }

    printDouble(Float32 recurse(Int32 60))
    printDouble(Float32 b.original)

    modifyStruct(b.data)
    printDouble(b.data.bigNumber)

    let structCopy = b.data
    b.data.bigNumber = Float32 getInteger()
    printDouble(structCopy.bigNumber)
    printDouble(b.data.bigNumber)

    printNumbers(Int32 10)

    let a = returnStruct()
    printDouble(Float32 a.x)
    printDouble(Float32 a.y)
}

export func printNumbers(max: Int32) {
    while (max > 0) {
        max = Int32 (max - 1)
        printDouble(Float32 max)
    }
}

func recurse(a: Int32): Int32 {
    if (a == 0) {
        return 0
    }
    else {
        return a + recurse(Int32 (a - 1))
    }
}