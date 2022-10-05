
export extern func printDouble(float: Float32): Float32

// export extern func malloc(bytes: UInt64): UInt64
// export extern func free(pointer: UInt64)

func getInteger(): Int64 {
    return 16
}

struct Person {
    age: Int32
    birthDate: Int64
}

func getStruct(): Person {
    return Person {
        age: 21,
        birthDate: 20010130
    }
}

export func main() {
    printDouble(Float32 getInteger())

    let person = getStruct()
    printDouble(Float32 person.age)
}

