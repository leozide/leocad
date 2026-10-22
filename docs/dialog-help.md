# Dialog tooltips and contextual help

Tooltips explain a control in one short sentence. Use sentence case, US English,
and a final period. State units where relevant. Explain the action or effect
instead of repeating the label. Standard OK and Cancel buttons need no tooltip.

Use `whatsThis` for longer explanations: what a setting changes, dependencies,
ranges, and a small example when useful. A dialog overview explains its purpose
and workflow; group help explains related fields. Verify behavior in the code
before documenting defaults, units, or limits. Keep essential instructions
visible in the dialog rather than available only on hover.

Keep static help in the dialog's `.ui` file so Qt extracts it for translation.
Use translated C++ strings for dynamic help. Do not translate static data before
application translators are installed. New catalog entries remain unfinished
until reviewed, following the existing translation policy. Refresh affected
catalogs after changing UI strings, preserving existing translations and review
status. Maintain `resources/leocad_en.ts` with `lupdate -pluralonly`. Keep the
current build policy of including nonempty unfinished translations.

Add the standard Help button to the dialog's button box, then call
`lcSetupDialogHelp()` after `setupUi()`. Help shows the dialog overview. F1 shows
help for the focused control, falling back through its parent groups to the
dialog overview. This also handles focus inside composite controls such as
spin boxes. Both entry points use Qt's native What's This popup.

Give unlabeled controls meaningful accessible names. Check Help, F1, and Escape
with keyboard navigation, as well as tooltip placement and longer translated
help. The initial examples are Array and Save Image.

Link field labels to their controls using Qt buddies. Buddies provide focus and
accessibility relationships, but do not share tooltips or contextual help.
Keep help on the control and copy its tooltip to the label in the `.ui` file.
Also copy field-specific `whatsThis` text when the label would otherwise fall
back to a different section or dialog overview. Identical text in the same
translation context shares a catalog entry. Update both properties together
when the help changes.
