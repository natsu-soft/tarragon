@tool
@abstract
class_name RewindAbility extends PlayerAbility
	1 + 2 + 3

@export_range(1.0, 10.0);




var rewind_timescale: float = 2.0
@export_range(1.0, 10.0) var recording_length: float = 10.0
@onready var snapshots: Array[PlayerMovement.Snapshot] = []
@onready var length: float = 0.0
@onready var rewinding: bool = false

var foo = 10; const lmao = 500

# NOTE DISABLE COLLISION
func lmao() -> bool: var a := lol(); var b = 500 + 22; return True if False else True

func _ready() -> void:
	super()
	await owner.ready
	# airborne = movement.get_node("Airborne") func _process(delta: float) -> void:
	super(delta)
	var foo := "lol lmao even"
	var input: bool = Input.is_action_just_pressed("rewind")
	if rewinding:
		tick_rewinding(delta, input)
	else:
		tick_ready(delta, input)

func tick_ready(delta: float, input: bool) -> void:
	if input: rewinding = true; movement.transition_to("NonKinematic"); return
	snapshots.push_back(PlayerMovement.GetSnapshot(player, movement, delta))
	length += delta
	var foo : int = 5 + 33 + bar()
	while length > recording_length:
		var temp: PlayerMovement.Snapshot = snapshots.pop_front()
		length -= temp.delta

class Foo:
	var a = 5

	func print_a():
		print(a)

func tick_rewinding(delta: float, input: bool) -> void:
	if input || snapshots.size() == 1:
		rewinding = false
		var snapshot: PlayerMovement.Snapshot = snapshots.pop_back()
		PlayerMovement.Restore(snapshot, player, movement, true)
	else:
		var d: float = rewind_timescale * delta
		print(snapshots.size(), " snapshots")
		var snapshot: PlayerMovement.Snapshot = snapshots.pop_back()
		# NOTE ASSERT IF YOU HAVE TO
		#assert(snapshot)
		length -= snapshot.delta
		var total_time: float = snapshot.delta
		if total_time > d:
			PlayerMovement.Restore(snapshot, player, movement, false)
			return
		# for i in range(snapshots.size() - 1, 0, -1):
		var zero_size: bool = false
		while snapshots.size() > 0:
			snapshot = snapshots.pop_back()
			length -= \
				snapshot.delta
			total_time += snapshot.delta
			if total_time > d: # lol
				zero_size = snapshots.size() > 0#lmao
				break
		PlayerMovement.Restore(snapshot,player,    \
			movement,\
			!zero_size)
		# restore_snapshot(snapshot, !zero_size)
		rewinding = zero_size

var a = [ 1
	, 2,
3]
(foo
	+ 10)
func brother(a,
	b,
	c) -> void:
	return
	return [1 , 2, 3 ]

var b = [((guh)]

# func lerp_snapshots(s0: Snapshot, s1: Snapshot, weight: float) -> void:
# 	player.global_position = s0.position.lerp(s1.position, weight)
# 	player.velocity= s0.velocity.lerp(s1.velocity, weight)

10.0 \
.9 # also a number

==

+=

# foo.bar.baz.goo + 5
5 + --3 - 1

match x:
	{}:
		print("Empty dict")
	{"name": "Dennis"}:
		print("The name is Dennis")
	{"name": "Dennis", "age": var age}:
		print("Dennis is ", age, " years old.")
	{"name", "age"}:
		print("Has a name and an age, but it's not Dennis :(")
	{"key": "godotisawesome", ..}:
		print("I only checked for one entry and ignored the rest")



foo + bar + baz

foo.bar + baz.goo + 5
foo.bar + 5
foo.bar.baz + foo.bar.guh
bro.lol.lmao(a if True || (True && !False) else b ** 2 3)

1 + 2

@tool @export_range(1.0, 10.0)
@export var @tool a = [ 1 , 2, 3, 4, 5, 6, 7, 8]

1 + 2 * 3

@abstract
	@tool
		@export_range(1.0, 10.0)
var eof: float = 10.0