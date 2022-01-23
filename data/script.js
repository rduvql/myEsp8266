//@ts-check

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




/**
* MAIN
*/
(() => {
    console.log("loaded !")
    
    var gwUrl = "ws://" + location.host + "/ws";
    var webSocket = new WebSocket(gwUrl);
    
    webSocket.onopen = function(openEvent) {
        console.log("open");
    }
    webSocket.onclose = function(closeEvent) {
        console.log("close");
    }
    webSocket.onmessage = function(msgEvent) {
        console.log("message", msgEvent.data);
        
        let p = document.createElement("p");
        p.innerText = msgEvent.data;
        
        let div = document.getElementById("list");
        div.appendChild(p);
        window.scrollTo(0, document.body.scrollHeight);
    }
    
    fetch(`/info`, { method: "GET" }).then((response) => {
        response.json().then(v => {
            console.log(v)
        })
    });
    
})();