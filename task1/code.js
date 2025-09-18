let x1Input = document.getElementById("x1")
let x2Input = document.getElementById("x2")
let funcInput = document.getElementById("func")
let button = document.getElementById("draw-button")

button.addEventListener("click", updateScreen)

let canvas = document.getElementById("plot-screen")
ctx = canvas.getContext('2d')

function resizeCanvas() {
    canvas.width = window.innerWidth * 0.9
    canvas.height = window.innerHeight * 0.8
    updateScreen()
}

window.addEventListener("resize", resizeCanvas)

resizeCanvas()

function updateScreen() {
    let func
    console.log("parsig func")
    try {
        const exprStr = funcInput.value
        const expr = math.parse(exprStr)      
        const f = expr.compile()              
        func = (x) => f.evaluate({ x: x }) 
        func(0)
    } catch (e) {
        canvas.style.border = "2px solid red";
        console.log("Function parsing error", e)
        return
    }

    const x1 = Number.parseFloat(x1Input.value)
    const x2 = Number.parseFloat(x2Input.value)
    if (isNaN(x1) || isNaN(x2) || x1 >= x2) {
        console.error("Invalid x range")
        return
    }
    drawGraph(func, x1, x2)
}


function drawGraph(func, x1, x2) {
        ctx.fillStyle = "white"
        ctx.fillRect(0, 0, canvas.width, canvas.height)

        let minY = Infinity, maxY = -Infinity
        let step = (x2 - x1) / canvas.width
        for (let i = 0; i <= canvas.width; i++) {
            let x = x1 + i * step
            let y = func(x)
            if (!isNaN(y) && isFinite(y)) {
                minY = Math.min(minY, y)
                maxY = Math.max(maxY, y)
            }
        }

        if (minY === Infinity || maxY === -Infinity) {
            canvas.style.border = "2px solid red";
            return
        }

        let scaleX = canvas.width / (x2 - x1)
        let scaleY = canvas.height / (maxY - minY)

        function toCanvasX(x) {
            return (x - x1) * scaleX
        }
        function toCanvasY(y) {
            return canvas.height - (y - minY) * scaleY
        }

        ctx.strokeStyle = "black"
        ctx.beginPath()
        if (0 >= x1 && 0 <= x2) {
            let x0 = toCanvasX(0)
            ctx.moveTo(x0, 0)
            ctx.lineTo(x0, canvas.height)
        }
        if (0 >= minY && 0 <= maxY) {
            let y0 = toCanvasY(0)
            ctx.moveTo(0, y0)
            ctx.lineTo(canvas.width, y0)
        }
        ctx.stroke()

        ctx.strokeStyle = "blue"
        ctx.beginPath()
        for (let i = 0; i <= canvas.width; i++) {
            let x = x1 + i * step
            let y = func(x)
            if (isNaN(y) || !isFinite(y)) continue
            let cx = toCanvasX(x)
            let cy = toCanvasY(y)
            if (i === 0) ctx.moveTo(cx, cy)
            else ctx.lineTo(cx, cy)
        }
        ctx.stroke()
        canvas.style.border = "2px solid green";
    }

function printPixel(x, y) {
    ctx.fillStyle = getRndColor()
    ctx.fillRect(x, y, 3, 3)
}

function getRndColor() {
    var r = 255*Math.random()|0, 
        g = 255*Math.random()|0,
        b = 255*Math.random()|0;
    return 'rgb(' + r + ',' + g + ',' + b + ')';
}

function sinx(x) {
    return Math.sin(x)
}

/*
imageData = ctx.createImageData(canvas.width, canvas.height)
data = imageData.data
console.log("script started")

color = 0

for (i = 0; i < data.lenght; i += 4) {
    data[i] = 255
    data[i + 1] = 0
    data[i + 2] = 0
    data[i + 3] = 255
}
color += 1


ctx.putImageData(data, 0, 0)
*/