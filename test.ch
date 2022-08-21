
func getNumber() {
    return 100
}

export func main() {

    let i = getNumber()
    if (i > 100) {
        i = i * 1000
    }
    
    return i

}