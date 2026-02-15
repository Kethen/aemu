const http = require('node:http');
const fs = require('node:fs');
const { Buffer } = require('node:buffer');
const fxparser = require("./fxparser/fxp.min.js");

const postoffice_data_url = "http://127.0.0.1:27314"
const server_port = 8080;

const xml_parser_options = {
	ignoreAttributes: false,
	isArray: (name, jpath, isLeafNode, isAttribute) => {
		let array_names = [
			"game",
			"group",
			"user"
		];

		for (const tag_name of array_names){
			if (tag_name == name){
				return true
			}
		}

		return false;
	}
};
const xml_parser = new fxparser.XMLParser(xml_parser_options);

process.on('SIGTERM', () => {
   process.exit(1); 
});

process.on('SIGINT', () => {
   process.exit(1); 
});

const server = http.createServer();

let last_good_xml_parse_result = {};
let last_good_postoffice_output = {};
let last_good_processed_data = {};

function fetch_status_xml(){
	fs.readFile("./status.xml", "utf8", (err, data) => {
		if (err) {
			console.log(`failed opening ./status.xml, ${err}`)
		}else{
			try{
				let output = xml_parser.parse(data, true);
				last_good_xml_parse_result = output;
				//console.log(JSON.stringify(last_good_xml_parse_result, null, 4));
			}catch(err){
				console.log(`failed parsing ./status.xml, ${err}`)
			}
		}
	})
}

function fetch_postoffice_output(){
	let data = [Buffer.alloc(0)];

	let req = http.get(postoffice_data_url, (res) => {
		res.on("data", (chunk) => {
			data[0] = Buffer.concat([data[0], chunk])
		});

		res.on("end", () => {
			try{
				let string_data = data[0].toString("utf8");
				try{
					let postoffice_output = JSON.parse(string_data)
					last_good_postoffice_output = postoffice_output;
					//console.log(JSON.stringify(last_good_postoffice_output, null, 4));
				}catch(err){
					console.log(`failed decoding data from aemu postoffice as json, ${err}, ${string_data}`);
				}
			}catch(err){
				console.log(`failed decoding data from aemu postoffice as utf8, ${err}`);
			}
		});
	});

	req.on("error", (e) => {
		console.log(`failed fetching data from aemu_postoffice`)
	});
}

function fetch_data(){
	fetch_status_xml();
	fetch_postoffice_output();
}

setInterval(fetch_data, 2000);

function process_data(res){
	let processed_data = {
		games:[]
	};

	let pro = last_good_xml_parse_result["prometheus"];
	if (pro == undefined){
		console.log(`prometheus not found in xml, please debug this`);
		return;
	}
	let games = pro["game"];
	if (games == undefined){
		last_good_processed_data = processed_data;
		return;
	}
	for(const game of games){
		let game_entry = {
			name:game["@_name"],
			usercount:game["@_usercount"],
			groups:[],
		};
		processed_data.games.push(game_entry)

		let groups = game["group"];
		if (groups == undefined){
			continue;
		}
		for (const group of groups){
			let group_entry = {
				name:group["@_name"],
				usercount:group["@_usercount"],
				users:[],
			};
			game_entry.groups.push(group_entry);

			let users = group["user"];
			if (users == undefined){
				continue;
			}
			for(const user of users){
				let user_entry = {
					name:user["#text"],
					pdp_ports:[],
					ptp_ports:[],
				};
				group_entry.users.push(user_entry);

				let postoffice_user = last_good_postoffice_output[user["@_mac_address"]];
				if (postoffice_user == undefined){
					continue;
				}
				for(const socket of postoffice_user){
					if (socket.state == "pdp"){
						user_entry.pdp_ports.push(socket.sport);
						continue;
					}
					if (socket.state == "ptp_listen"){
						user_entry.ptp_ports.push(socket.sport);
						continue;
					}
				}
			}
		}
	}

	last_good_processed_data = processed_data;

	//console.log(JSON.stringify(last_good_processed_data, null, 4));
}

setInterval(process_data, 5000);

function draw_page(res){
}

const resources = [
	{
		path:"/favicon.ico",
		type:"image/ico"
	},
	{
		path:"/spinning.gif",
		type:"image/gif"
	},
	{
		path:"/bg.jpg",
		type:"image/jpeg"
	},
	{
		path:"/titlebg.png",
		type:"image/png"
	},
	{
		path:"/status.html",
		type:"text/html; charset=utf-8"
	},
	{
		path:"/style.css",
		type:"text/css; charset=utf-8"
	},
]

function request_handler(req, res){
	let url = req.url;
	if (url == "/"){
		url = "/status.html";
	}
	switch(url){
		case "/data.json":{
			res.writeHead(200, {"Content-Type": "application/json", "Access-Control-Allow-Origin": "*"});
			res.end(JSON.stringify(last_good_processed_data));
			break;
		}
		default:{
			let resource_found = false;
			for(const resource_entry of resources){
				if (url == resource_entry.path){
					fs.readFile("." + url, null, (err, data) => {
						if (err){
							console.log(`error ${err} while reading ${url}`)
							res.writeHead(500);
							res.end(`failed reading ${url}`);
						}else{
							res.writeHead(200, {"Content-Type": resource_entry.type});
							res.end(data);
						}
					})
					resource_found = true;
					break;
				}
			}

			if (!resource_found){
				res.writeHead(400);
				res.end(`${url} not found`);
			}
		}
	}
}

server.on("request", request_handler);

server.listen(server_port);
