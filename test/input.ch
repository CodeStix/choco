type List<T> { 
    items: heap T
    length: T[10, 4]
} 

type ConsoleBuffer = UInt8[4, 10]



// Type is Color value[10, 3]
const BUFFER = Color value[
    [fromRGG(200,255,255), fromRGG(255,200,200), fromRGG(255,255,255)],
    [fromRGG(255,255,255), fromRGG(255,255,255), fromRGG(255,200,255)],
    [fromRGG(255,200,255), fromRGG(255,200,255), fromRGG(255,255,255)],
    [fromRGG(255,200,255), fromRGG(200,255,255), fromRGG(255,255,200)],
    [fromRGG(255,255,200), fromRGG(255,255,200), fromRGG(255,255,255)],
    [fromRGG(255,255,255), fromRGG(255,255,255), fromRGG(255,255,255)],
    [fromRGG(200,255,200), fromRGG(255,255,255), fromRGG(255,255,255)],
    [fromRGG(200,255,255), fromRGG(255,255,255), fromRGG(255,200,255)],
    [fromRGG(255,255,255), fromRGG(255,255,255), fromRGG(255,255,255)],
    [fromRGG(255,255,255), fromRGG(255,200,255), fromRGG(255,255,255)],
]

let buffer = [
    UInt(8) __times 100
]



export func main() -> Person|Error {

    // will allocate a stack array
    let people = Person?[100] stack unmanaged[2] {
        {Person.new("stijn"), Person.new("rogiest"), Person.new("stijn")},
        {Person.new("stijn"), Person.new("rogiest"), Person.new("stijn"), Person.new("rogiest")}
    }

    let people = unmanaged[[Person? # 100] # 2]

    let people = [
        Person.new("stijn", "rogiest")
    ]:[Person # 100]

    let num = 100:Int32

    let person = unmanaged Person {
        firstName: "stijn",
        lastName: "rogiest"
    }

    let ints = unmanaged[Int] [0, 1, 2, 3];

    let values = getValues("...") return Error;

    let people = [Person] [0]; 

}

extern func bruh() {}