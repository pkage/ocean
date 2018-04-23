const clock = () => {
	fetch('/api/time')
		.then(r => r.json())
		.then(r => {
			const t = new Date(r.time * 1000)

			const time = document.querySelector('#clock')
			time.textContent = `${t.getHours()}:${t.getMinutes()}:${t.getSeconds()}`
			setTimeout(clock, 100)
		})
}

clock()
