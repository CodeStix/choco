
export extern func printDouble(float: Float32): Float32

// export extern func malloc(bytes: UInt64): UInt64
// export extern func free(pointer: UInt64)

struct Person {
    age: Int32
    birthDate: Int64
}

struct MultiplePeople {
    person1: Person
    person2: Person
    person3: Person
    person4: Person
}

export func getStruct(): Person {
    let p = Person {
        age: 21,
        birthDate: 20010130
    }
    printDouble(Float32 p.refs)
    return p
}

export func duplicatePerson(person: Person): MultiplePeople {
    return MultiplePeople {
        person1: person
        person2: person
        person3: person
        person4: person
    }
}

export func main() {
    let person = getStruct()
    printDouble(Float32 person.refs)
    printDouble(Float32 person.age)

    let duplicate = duplicatePerson(person)
    printDouble(Float32 duplicate.person1.birthDate)
    printDouble(Float32 duplicate.person2.birthDate)
    printDouble(Float32 duplicate.person3.birthDate)
    printDouble(Float32 duplicate.person4.birthDate)
}

