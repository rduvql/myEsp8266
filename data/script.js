
/**
 * @param input {HTMLInputElement}
 */
function request(input){
    let status = input.checked ? "ON" :"OFF";

    // const myURL = new URL('https://example.org/?abc=123');
    // console.log(myURL.searchParams.get('abc'));
    // myURL.searchParams.append('abc', 'xyz');
    // console.log(myURL.href);

    fetch(`/${input.id}=${status}`, {
        method: "POST"
    })
    .then((response) => {
        response.text().then(v => {
            console.log(v)
        })
    });
}

(() => {
    console.log("loaded !")

    fetch(`/info`, {
        method: "GET"
    })
    .then((response) => {
        response.json().then(v => {
            console.log(v)
        })
    });
    
})();