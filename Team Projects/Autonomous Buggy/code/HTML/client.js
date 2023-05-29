/* Quest 6: Rally
   BU ENG EC 444 Spring 2023
   Team 2: John Culley, Ananth Sanjay, Jeffrey Zhang, Kai Imery
   04/27/2023 
*/

$(function () {
    var btn_status = { id: "", val: "" };

    $(document).on("keydown", function(event) {
        switch(event.key) {
            case "w":
                btn_status.id = "btn1";
                btn_status.val = "1";
                break;
            case "s":
                btn_status.id = "btn1";
                btn_status.val = "2";
                break;
            case "d":
                btn_status.id = "btn1";
                btn_status.val = "3";
                break;
            case "a":
                btn_status.id = "btn1";
                btn_status.val = "4";
                break;
            case "q":
                btn_status.id = "btn1";
                btn_status.val = "5";
                break;
            case "1":
                btn_status.id = "btn1";
                btn_status.val = "6";
                break;
            case "2":
                btn_status.id = "btn1";
                btn_status.val = "7";
                break;
            case "3":
                btn_status.id = "btn1";
                btn_status.val = "8";
                break;
            case "`":
                btn_status.id = "btn1";
                btn_status.val = "9";
                break;
            default:
                btn_status.id = "btn1";
                btn_status.val = "0";
                return;
        }
        var socket = new WebSocket("ws://localhost:8082");
        socket.addEventListener('open', function (event) {
            socket.send(JSON.stringify(btn_status));
        });
        socket.addEventListener('message', function (event) {
            console.log('Server response:', event.data);
            socket.close();
        });
    });
});


