<h1>SmartMirror</h1>
						<h2>Description</h2>
						<p>
							SmartMirror is a DIY project. Basically it is an LED-Matrix of 64x16 placed behind a translucent mirror. It is powered by a NodeMCU chip, so it has WiFi capabilities. It can show the time in some special decimal form (where the day has 10 hours, a hour 100 minutes and a minute 100 seconds) but a normal 24-hour format is also possible. A DHT22 temperature and humidity sensor provides ambient values. Through implementation of a simple HTTP server, the mirror can react to GET requests. It gathers the current time via NTP (thus it needs to be connected to the Internet).
						</p>
						<p>
							Required libraries:<br>
							<ul>
								<li><a href="https://github.com/Seeed-Studio/Ultrathin_LED_Matrix">Ultrathin_LED_Matrix by Seeed-Studio</a></li>
								<li><a href="https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTstable">DHTstable by RobTillaart</a></li>
								<li><a href="https://github.com/PaulStoffregen/Time">Time by PaulStoffregen</a></li>
								<li><a href="https://github.com/JChristensen/Timezone">Timezone by JChristensen</a></li>
							</ul>
						</p>
						<h2>GET request modes</h2>
						<p>
							<table class="bordered">
								<tr><td>/clock10</td><td>Shows the clock in decimal mode.</td></tr>
								<tr><td>/clock24</td><td>Shows the clock in 24-hour format.</td></tr>
								<tr><td>/temperature</td><td>Returns the temperature.</td></tr>
								<tr><td>/humidity</td><td>Returns the humidity.</td></tr>
								<tr><td>/disable</td><td>Shuts off the display (but continues to respond to requests).</td></tr>
								<tr><td>/enable</td><td>Turns the display on.</td></tr>
								<tr><td>/text?</td><td>Shows text on the screen.<br>
														style=[ marquee | rightbound | leftbound | center ], default is marquee<br>
														In case of marquee: direction=[ right | left ], default is left; speed=THE HIGHER, THE SLOWER<br></td></tr>
							</table>
						</p>
						<h2>Further information</h2>
						<p>
							More information can be found on the <a href="http://bengelhaupt.com/projects/smartmirror">project website</a>
						</p>
